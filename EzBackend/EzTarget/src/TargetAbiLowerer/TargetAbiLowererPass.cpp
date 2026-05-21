#include "TargetAbiLowerer/TargetAbiLowererPass.h"
#include "Instruction/MirInstructionDefs.h"

TargetAbiLowererPass::TargetAbiLowererPass(TargetAbiLowererContext *ctx) : m_ctx(ctx) {}

bool TargetAbiLowererPass::run(TypedPoolLinkedList<class MirFunction> *funcList,
                               TypedPoolLinkedList<class MirFunction>::Iterator it,
                               class MirPassManager *passManager)
{
    MirEmitter *emitter = m_ctx->getEmitter();
    MirEmitterContext *emitterContext = emitter->getContext();
    MirFunction *func = *it;
    TargetDesc *targetDesc = m_ctx->getTargetDesc();
    ABIDesc *abi = targetDesc->getABI();
    bool modified = false;

    // Extract the abstract parameters of this function and map them to physical
    // hardware registers or stack load instructions at the top of the entry block.
    if (func->getParameters()->m_numElems != 0)
    {
        modified |= lowerParameters(abi, emitter, emitterContext, func);
    }

    // Iterate through all instructions in the function looking for ABI boundaries (CALL and RET).
    for (auto blockIt = func->getBlocks()->begin(); blockIt != func->getBlocks()->end(); ++blockIt)
    {
        MirBlock *block = *blockIt;
        TypedPoolLinkedList<MirInstruction> *instrList = block->getInstructions();

        for (auto instIt = instrList->begin(); instIt != instrList->end(); ++instIt)
        {
            MirInstruction *instr = *instIt;
            MirInstructionOpCode opcode = instr->getOpCode();

            if (opcode == MirInstructionOpCode::CALL)
            {
                // We are calling another function. We must move our virtual arguments
                // into physical ABI registers just before the CALL instruction.
                modified |= lowerCallSite(block, instIt, abi, emitter, emitterContext);
            }
            else if (opcode == MirInstructionOpCode::RET)
            {
                // We are exiting the function. We must move our virtual return value
                // into the physical ABI return register (e.g., RAX or A0) before RET.
                modified |= lowerReturnSite(block, instIt, abi, emitter, emitterContext);
            }
        }
    }

    return modified;
}

MirPassIterationPlace TargetAbiLowererPass::getIterationPlace() const { return MirPassIterationPlace::Function; }

bool TargetAbiLowererPass::lowerParameters(ABIDesc *abi,
                                           MirEmitter *emitter,
                                           MirEmitterContext *emitterCtx,
                                           MirFunction *func)
{
    MirBlock *funcEntryPoint = func->getEntryPoint();

    // We want to insert these ABI loads at the very top of the block.
    // If the block already has instructions, we insert before the first one.
    auto instructions = funcEntryPoint->getInstructions();
    if (instructions->m_numElems > 0)
        emitterCtx->setInsertPoint(funcEntryPoint, instructions->begin());
    else
        emitterCtx->setInsertPoint(funcEntryPoint); // Empty block, just append AFTER

    size_t index = 0;
    bool modified = false;

    for (auto it = func->getParameters()->begin(); it != func->getParameters()->end(); ++it, index++)
    {
        MirOperand *op = *(*it);
        ArgLocation loc = abi->getArgLoc(index, op->getMirType());

        // Safety check: Parameters at this level are always virtual registers
        if (!op->isOfType<MirRegister>())
        {
            throw std::runtime_error("Internal Compiler Error: Given function parameter is not a register.");
        }

        MirRegister *paramReg = op->get<MirRegister>();
        MirType *paramType = paramReg->getMirType();

        if (loc.isPhysicalReg())
        {
            // The parameter is encoded in a specific physical register (e.g., RCX, RDI).
            PhysicalRegLocation physReg = loc.getPhysicalLoc();

            // Create a physical register operand mapped to the hardware ID
            MirRegister *physRegOp = emitter->createPhysicalRegister(paramType, physReg.m_id);

            // Emit: paramReg = MOV physRegOp
            emitter->emit(MirInstructionOpCode::MOV, { paramReg, physRegOp });
            modified = true;
        }
        else if (loc.isStack())
        {
            // Ask the Frame Builder to track this physical memory and give us a tracking ID.
            size_t paramSize = paramReg->getSizeInBytes();
            StackFrameObject *obj = func->getStackFrame()->createParam(paramSize,
                                                                       paramSize, // Using size as alignment
                                                                       loc.getStackLoc().m_offset);

            // Create the Frame Index operand representing the stack slot
            MirFrameIndex *frameIndex = emitter->createFrameIndex(paramType, obj->m_id);

            // Wrap it in a Memory operand because LOAD requires a memory access constraint
            MirMemory *memAccess = emitter->createMemoryOperand(paramType, frameIndex, nullptr);

            // Emit: paramReg = LOAD memAccess
            emitter->emit(MirInstructionOpCode::LOAD, { paramReg, memAccess });
            modified = true;
        }
        else if (loc.isSplit())
        {
            // TODO: Allow splitting an argument into multiple registers.
        }
    }

    // Restore the insertion point to append mode in case further passes use this context
    emitterCtx->setInsertPoint(funcEntryPoint);

    return modified;
}

bool TargetAbiLowererPass::lowerCallSite(MirBlock *block,
                                         TypedPoolLinkedList<MirInstruction>::Iterator it,
                                         ABIDesc *abi,
                                         MirEmitter *emitter,
                                         MirEmitterContext *emitterCtx)
{
    MirInstruction *callInstr = *it;
    auto operands = callInstr->getOperands();
    bool modified = false;

    // Lock the insertion point so our ABI setup instructions appear BEFORE the CALL
    emitterCtx->setInsertPoint(block, it);

    // Operand 0 is the call target. Operands 1..N are the arguments.
    auto opIt = operands->begin();

    // Safety check: Ensure there is at least a target
    if (opIt == operands->end())
    {
        throw std::runtime_error("Internal Compiler Error: Call did not have callSite to lower with ABI rules");
    }
    ++opIt; // Skip the target operand to start looking at arguments

    size_t argIndex = 0;
    while (opIt != operands->end())
    {
        MirOperand *argOp = *opIt;
        MirType *argType = argOp->getMirType();
        ArgLocation loc = abi->getArgLoc(argIndex, argType);

        if (loc.isPhysicalReg())
        {
            PhysicalRegLocation physReg = loc.getPhysicalLoc();
            MirRegister *physRegOp = emitter->createPhysicalRegister(argType, physReg.m_id);

            // Emit: MOV physReg, virtArg
            emitter->emit(MirInstructionOpCode::MOV, { physRegOp, argOp });

            // IN-PLACE SWAP: Replace the virtual argument in the CALL with the physical register.
            // This acts as a "Use Anchor" so the Register Allocator knows this hardware register
            // must stay alive at the exact moment the CALL happens!
            opIt.m_curr->m_object = physRegOp;

            ++opIt;
            modified = true;
        }
        else if (loc.isStack())
        {
            // Fetch the type sized for a pointer/register on this ABI
            size_t ptrSize = abi->getRegSizeInBits() / 8;
            MirType *ptrType = emitterCtx->getIntegerTypeBySize(ptrSize);

            // Create operands for [StackReg + Offset]
            MirRegister *spReg = emitter->createPhysicalRegister(ptrType, abi->getStackReg());
            MirInteger *offset = emitter->createImmediateInteger(ptrType, loc.getStackLoc().m_offset);
            MirMemory *memAccess = emitter->createMemoryOperand(argType, spReg, offset);

            // Emit: STORE [SP + Offset], virtArg
            emitter->emit(MirInstructionOpCode::STORE, { memAccess, argOp });

            // Because the argument is safely in memory, it doesn't need to be passed
            // as an operand to the CALL instruction anymore. We remove it from the list.
            // (Note: removeFromList returns the NEXT iterator, making it safe for while-loops)
            opIt = emitterCtx->getOperandPool()->removeFromList(operands, opIt);
            modified = true;
        }
        else if (loc.isSplit())
        {
            // TODO: Split logic
            ++opIt;
        }

        argIndex++;
    }

    // Restore insertion point
    emitterCtx->setInsertPoint(block);
    return modified;
}

bool TargetAbiLowererPass::lowerReturnSite(MirBlock *block,
                                           TypedPoolLinkedList<MirInstruction>::Iterator it,
                                           ABIDesc *abi,
                                           MirEmitter *emitter,
                                           MirEmitterContext *emitterCtx)
{
    MirInstruction *retInstr = *it;
    auto operands = retInstr->getOperands();

    // If there are 0 operands, it's a void return, so there is no ABI data to move.
    if (operands->m_numElems == 0)
        return false;

    // Lock the insertion point BEFORE the RET instruction
    emitterCtx->setInsertPoint(block, it);

    MirOperand *retValue = *operands->begin();
    const ArgLocation &loc = abi->getReturnValueLoc();

    if (loc.isPhysicalReg())
    {
        PhysicalRegLocation physReg = loc.getPhysicalLoc();
        MirRegister *physRegOp = emitter->createPhysicalRegister(retValue->getMirType(), physReg.m_id);

        // Emit: MOV physRetReg, retValue
        emitter->emit(MirInstructionOpCode::MOV, { physRegOp, retValue });
        operands->m_owner->removeFromList(operands, operands->begin());
    }
    else if (loc.isStack())
    {
        // TODO: Handle hidden return pointers for large structs (sret). Add a new instruction called "sret" or
        // something.
    }

    // Restore insertion point
    emitterCtx->setInsertPoint(block);
    return true;
}

const char *TargetAbiLowererPass::getName() const { return "TargetAbiLowererPass"; }
