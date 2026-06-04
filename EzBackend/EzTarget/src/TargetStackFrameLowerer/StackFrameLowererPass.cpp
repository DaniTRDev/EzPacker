#include "TargetStackFrameLowerer/StackFrameLowerer.h"
#include <unordered_set>
#include <vector>
#include <algorithm>

/**
 * Helper function to replace all abstract FrameIndex memory operands with
 * physical Base + Displacement operands relative to the Frame Pointer.
 */
static void rewriteFrameIndices(MirEmitter *emitter, ABIDesc *abiDesc, MirFunction *func)
{
    MirType *ptrType = emitter->getContext()->getIntegerTypeBySize(abiDesc->getRegSizeInBits() / 8);
    MirRegister *fpReg = emitter->createPhysicalRegister(ptrType, abiDesc->getStackFrameReg());
    int64_t ptrSize = abiDesc->getRegSizeInBits() / 8;

    auto stackObjList = func->getStackFrame()->getStackFrameObjects();

    for (MirBlock *block : *func->getBlocks())
    {
        for (MirInstruction *instr : *block->getInstructions())
        {
            auto operands = instr->getOperands();
            for (auto opIt = operands->begin(); opIt != operands->end(); ++opIt)
            {
                MirOperand *op = *opIt;
                if (op && op->isOfType<MirMemory>())
                {
                    MirMemory *mem = op->get<MirMemory>();

                    // Look for memory accesses where the base is a FrameIndex
                    if (mem->getBase() && mem->getBase()->isOfType<MirFrameIndex>())
                    {
                        MirFrameIndex *frameIdx = mem->getBase()->get<MirFrameIndex>();

                        // Look up the specific stack frame object by ID
                        StackFrameObject *targetObj = nullptr;
                        for (auto objIt = stackObjList->begin(); objIt != stackObjList->end(); ++objIt)
                        {
                            if ((*objIt)->m_id == frameIdx->getFrameId())
                            {
                                targetObj = *objIt;
                                break;
                            }
                        }

                        if (!targetObj)
                            continue;

                        int64_t displacement = 0;

                        // Identify Parameters vs Locals based on the source value.
                        // Based on your logs, source == 0 is Parameter, source == 2 is Spill.
                        if (targetObj->m_source == StackFrameObjectSource::Parameter)
                        {
                            // PARAMETERS: Reside ABOVE the saved RBP and Return Address.
                            // Address = RBP + (2 * PtrSize) + LogicalOffset
                            displacement = targetObj->m_offset + (ptrSize * 2);
                        }
                        else
                        {
                            // LOCALS & SPILLS: Reside BELOW the physical RBP.
                            // Address = RBP - (LogicalOffset + Size)
                            displacement = -(static_cast<int64_t>(targetObj->m_offset + targetObj->m_sizeInBytes));
                        }

                        // Create the physical immediate displacement
                        MirInteger *displImm = emitter->createImmediateInteger(ptrType, displacement);

                        // Replace the abstract FrameIndex memory with physical RBP + Displacement
                        MirMemory *newMem = emitter->createMemoryOperand(mem->getMirType(), fpReg, displImm);
                        opIt.m_curr->m_object = newMem;
                    }
                }
            }
        }
    }
}

StackFrameLowererPass::StackFrameLowererPass(StackFrameLowererContext *ctx) : m_ctx(ctx) {}

bool StackFrameLowererPass::run(TypedPoolLinkedList<struct MirFunction> *funcList,
                                TypedPoolLinkedList<struct MirFunction>::Iterator it,
                                struct MirPassManager *passManager)
{
    MirFunction *func = *it;
    MirBuilderContext *ctx = m_ctx->getEmitter()->getContext();

    // 1. Calculate offsets for locals and spills
    if (!calculateStackFrameOffsets(func))
    {
        return false;
    }

    // 2. Insert the Prologue at the very beginning of the Entry Block
    MirBlock *entryBlock = func->getEntryPoint();
    if (entryBlock->getInstructions()->m_numElems > 0)
        ctx->setInsertPoint(entryBlock, entryBlock->getInstructions()->begin());
    else
        ctx->setInsertPoint(entryBlock);

    insertPrologue();

    // 3. Trace every RET instruction to insert the Epilogue
    for (MirBlock *block : *func->getBlocks())
    {
        auto *instrList = block->getInstructions();
        if (instrList->m_numElems == 0)
            continue;

        auto *tailNode = instrList->m_tail;
        MirInstruction *lastInstr = tailNode->m_object;

        if (lastInstr->getOpCode() == MirInstructionOpCode::RET)
        {
            TypedPoolLinkedList<MirInstruction>::Iterator lastInstrIt{ tailNode };
            ctx->setInsertPoint(block, lastInstrIt);
            insertEpilogue();
        }
    }

    // 4. CRITICAL: Rewrite all abstract FrameIndices into physical RBP addresses
    rewriteFrameIndices(m_ctx->getEmitter(), m_ctx->getTargetDesc()->getABI(), func);

    return true;
}

bool StackFrameLowererPass::calculateStackFrameOffsets(MirFunction *func)
{
    auto stackObjList = func->getStackFrame()->getStackFrameObjects();
    int64_t currentLocalOffset = 0;

    for (auto objIt = stackObjList->begin(); objIt != stackObjList->end(); ++objIt)
    {
        StackFrameObject *obj = *objIt;

        // Parameters already have fixed positive offsets assigned by the ABI lowerer.
        // We skip them because they live in a completely different physical area (above RBP).
        if (obj->m_source == StackFrameObjectSource::Parameter)
            continue;

        // For locals and spills, pack them sequentially below RBP.
        // Ensure the offset is aligned to the object's natural size (max 8-byte alignment)
        int64_t align = obj->m_sizeInBytes;
        if (align > 8)
            align = 8;
        if (align == 0)
            align = 1;

        currentLocalOffset = (currentLocalOffset + align - 1) & ~(align - 1);

        // Assign the logical offset (which rewriteFrameIndices will translate to negative)
        obj->m_offset = currentLocalOffset;

        // Advance by size for the next object
        currentLocalOffset += obj->m_sizeInBytes;
    }

    // ABI strict requirement: Final stack frame size MUST be 16-byte aligned.
    m_stackFrameEndOffset = (currentLocalOffset + 15) & ~15;

    return true;
}

bool StackFrameLowererPass::insertPrologue()
{
    ABIDesc *abiDesc = m_ctx->getTargetDesc()->getABI();
    MirEmitter *emitter = m_ctx->getEmitter();
    MirType *regType = emitter->getContext()->getIntegerTypeBySize(abiDesc->getRegSizeInBits() / 8);

    MirRegister *sp = emitter->createPhysicalRegister(regType, abiDesc->getStackReg());
    MirRegister *fp = emitter->createPhysicalRegister(regType, abiDesc->getStackFrameReg());
    int64_t fpSize = abiDesc->getRegSizeInBits() / 8;

    MirInteger *fpSizeImm = emitter->createImmediateInteger(regType, fpSize);
    emitter->emitSUB(sp, fpSizeImm);

    MirInteger *zeroOffset = emitter->createImmediateInteger(regType, 0);
    MirMemory *mem = emitter->createMemoryOperand(regType, sp, zeroOffset);
    emitter->emitSTORE(mem, fp);

    emitter->emitMOV(fp, sp);

    // Only allocate space if we actually have locals/spills
    if (m_stackFrameEndOffset > 0)
    {
        MirInteger *frameSizeImm = emitter->createImmediateInteger(regType, m_stackFrameEndOffset);
        emitter->emitSUB(sp, frameSizeImm);
    }

    return true;
}

bool StackFrameLowererPass::insertEpilogue()
{
    ABIDesc *abiDesc = m_ctx->getTargetDesc()->getABI();
    MirEmitter *emitter = m_ctx->getEmitter();
    MirType *regType = emitter->getContext()->getIntegerTypeBySize(abiDesc->getRegSizeInBits() / 8);

    MirRegister *sp = emitter->createPhysicalRegister(regType, abiDesc->getStackReg());
    MirRegister *fp = emitter->createPhysicalRegister(regType, abiDesc->getStackFrameReg());
    int64_t fpSize = abiDesc->getRegSizeInBits() / 8;

    emitter->emitMOV(sp, fp);

    MirInteger *zeroOffset = emitter->createImmediateInteger(regType, 0);
    MirMemory *mem = emitter->createMemoryOperand(regType, sp, zeroOffset);
    emitter->emitLOAD(fp, mem);

    MirInteger *fpSizeImm = emitter->createImmediateInteger(regType, fpSize);
    emitter->emitADD(sp, fpSizeImm);

    return true;
}

MirPassIterationPlace StackFrameLowererPass::getIterationPlace() const { return MirPassIterationPlace::Function; }
const char *StackFrameLowererPass::getName() const { return "StackFrameLowererPass"; }