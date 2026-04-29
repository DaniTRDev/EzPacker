#include "MirAbiLegalizerPass/MirAbiLegalizerPass.h"

MirAbiLegalizerPass::MirAbiLegalizerPass(MirLegalizerContext *ctx) : m_ctx(ctx) {}

bool MirAbiLegalizerPass::run(MirFunction *func, struct MirPassManager *passManager)
{
    bool changed = false;
    for (MirBlock *block : *func->getBlocks())
    {
        changed |= runOnBlock(block);
    }
    return changed;
}

bool MirAbiLegalizerPass::runOnBlock(MirBlock *block)
{
    bool changed = false;

    TypedPoolSlice<MirInstruction> *instrList = block->getInstructions();
    for (TypedPoolSlice<MirInstruction>::Iterator it = instrList->begin(); it != instrList->end(); ++it)
    {
        changed |= runOnInstruction(it, block);
    }

    return changed;
}

bool MirAbiLegalizerPass::runOnInstruction(TypedPoolSlice<MirInstruction>::Iterator instrIt, MirBlock *parentBlock)
{
    ABIDesc *abiDesc = m_ctx->getAbiDesc();
    MirEmitter *emitter = m_ctx->getEmitter();
    MirEmitterContext *emitterCtx = emitter->getContext();
    MirInstruction *instr = *instrIt;
    MirInstructionOpCode opcode = instr->getOpCode();
    TypedPool *instrPool = parentBlock->getInstructions()->m_owner;
    TypedPool *operandPool = instr->getOperands()->m_owner;

    // TODO: Extract common code by creating a "runOnOperand".

    switch (opcode)
    {
        case MirInstructionOpCode::CALL:
        {
            MirBlock *currentBlock = emitterCtx->getCurrentBoundBlock();
            emitterCtx->bindToBlock(nullptr);

            TypedPoolSlice<MirOperand> *operandList = instr->getOperands();
            TypedPoolSlice<MirOperand>::Iterator operandIt = operandList->begin();

            ++operandIt; // Skip the function target operand
            size_t index = 1;

            while (operandIt)
            {
                MirOperand *operand = *operandIt;
                const ArgLocation &loc = abiDesc->getArgLoc(index);

                if (loc.isPhysicalReg())
                {
                    MirRegister physReg = emitter->createPhysicalRegister(loc.getPhysicalLoc().m_id,
                                                                          loc.getPhysicalLoc().m_sizeInBits);
                    MirInstruction *mov = emitter->emitMOV(physReg, operand->getVariant());

                    instrPool->appendToSliceBefore(parentBlock->getInstructions(), instrIt, mov);
                    operandIt = operandPool->removeFromSlice(operandList, operandIt); // Let the call to be alone.
                }
                else if (loc.isStack())
                {
                    StackOffset offset = loc.getStackLoc().m_offset;

                    MirRegister physStackReg =
                            emitter->createPhysicalRegister(abiDesc->getStackReg(), abiDesc->getRegSizeInBits() / 8);

                    MirInstruction *store =
                            emitter->emitSTORE(physStackReg,
                                               MirInteger{ offset, abiDesc->getStackOffsetSizeInBits() / 8 },
                                               operand->getVariant());

                    instrPool->appendToSliceBefore(parentBlock->getInstructions(), instrIt, store);
                    operandIt = operandPool->removeFromSlice(operandList, operandIt);
                }
                else if (loc.isSplit())
                {
                    const std::vector<PhysicalRegLocation> &physRegs = loc.getSplitLoc().m_regs;

                    // The Type Legalizer should have given us the exact same number of chunks
                    if (operandList->m_numElems != physRegs.size())
                    {
                        throw std::runtime_error(
                                "Internal Compiler Error: RET operand count does not match ABI split register count!");
                    }

                    TypedPoolSlice<MirOperand>::Iterator chunkIt = operandList->begin();

                    // Iterate through the chunks and physical registers simultaneously
                    for (size_t i = 0; i < physRegs.size(); ++i)
                    {
                        MirOperand *chunkOperand = *chunkIt;
                        const PhysicalRegLocation &targetPhysLoc = physRegs[i];

                        // Create the physical register (e.g., $v0 for the first iteration, $v1 for the second)
                        MirRegister physReg =
                                emitter->createPhysicalRegister(targetPhysLoc.m_id, targetPhysLoc.m_sizeInBits / 8);

                        // Emit: MOV $v0, chunk0
                        MirInstruction *mov = emitter->emitMOV(physReg, chunkOperand->getVariant());
                        instrPool->appendToSliceBefore(parentBlock->getInstructions(), instrIt, mov);

                        // Replace the virtual chunk with the physical chunk in the RET instruction
                        // so Liveness Analysis keeps it alive!
                        chunkOperand->swapData(physReg);
                        ++chunkIt;
                    }
                }

                index++;
            }

            emitterCtx->bindToBlock(currentBlock);
            return true;
        }
        case MirInstructionOpCode::RET:
        {
            MirBlock *currentBlock = emitterCtx->getCurrentBoundBlock();
            emitterCtx->bindToBlock(nullptr);

            TypedPoolSlice<MirOperand> *operandList = instr->getOperands();
            if (operandList->m_numElems == 0)
                return false;

            const ArgLocation &resLoc = abiDesc->getReturnValueLoc();
            TypedPoolSlice<MirOperand>::Iterator operandIt = operandList->begin();
            MirOperand *operand = *operandIt;

            if (resLoc.isPhysicalReg())
            {
                MirRegister physReg = emitter->createPhysicalRegister(resLoc.getPhysicalLoc().m_id,
                                                                      resLoc.getPhysicalLoc().m_sizeInBits);
                MirInstruction *mov = emitter->emitMOV(physReg, operand->getVariant());
                instrPool->appendToSliceBefore(parentBlock->getInstructions(), instrIt, mov);

                // Replace virtual return value with physical return register
                operand->swapData(physReg);
            }
            else if (resLoc.isStack())
            {
                /*
                 * TODO:
                 * FIX BUG. Type legalizer is good -> RET64 in a 32bit arch -> RET chunk0, chunk1; ABI legalizer doesn't
                 * realise the rest of the operands that need to be set in stack as a part of the return.
                 */
                StackOffset offset = resLoc.getStackLoc().m_offset;

                // Returns usually use Frame Pointer if the caller allocated the return space,
                // but verify your specific ABI conventions!
                MirRegister physStackFrameReg =
                        emitter->createPhysicalRegister(abiDesc->getStackFrameReg(), abiDesc->getRegSizeInBits() / 8);

                MirInstruction *store =
                        emitter->emitSTORE(physStackFrameReg,
                                           MirInteger{ offset, abiDesc->getStackOffsetSizeInBits() / 8 },
                                           operand->getVariant());

                instrPool->appendToSliceBefore(parentBlock->getInstructions(), instrIt, store);

                // Safe to remove since it's the only/last item, but still catch the return value
                operandPool->removeFromSlice(operandList, operandIt);
            }
            else if (resLoc.isSplit())
            {
                const std::vector<PhysicalRegLocation> &physRegs = resLoc.getSplitLoc().m_regs;

                // The Type Legalizer should have given us the exact same number of chunks
                if (operandList->m_numElems != physRegs.size())
                {
                    throw std::runtime_error(
                            "Internal Compiler Error: RET operand count does not match ABI split register count!");
                }

                TypedPoolSlice<MirOperand>::Iterator chunkIt = operandList->begin();

                // Iterate through the chunks and physical registers simultaneously
                for (size_t i = 0; i < physRegs.size(); ++i)
                {
                    MirOperand *chunkOperand = *chunkIt;
                    const PhysicalRegLocation &targetPhysLoc = physRegs[i];

                    // Create the physical register (e.g., $v0 for the first iteration, $v1 for the second)
                    MirRegister physReg =
                            emitter->createPhysicalRegister(targetPhysLoc.m_id, targetPhysLoc.m_sizeInBits / 8);

                    // Emit: MOV $v0, chunk0
                    MirInstruction *mov = emitter->emitMOV(physReg, chunkOperand->getVariant());
                    instrPool->appendToSliceBefore(parentBlock->getInstructions(), instrIt, mov);

                    // Replace the virtual chunk with the physical chunk in the RET instruction
                    // so Liveness Analysis keeps it alive!
                    chunkOperand->swapData(physReg);
                    ++chunkIt;
                }
            }

            emitterCtx->bindToBlock(currentBlock);
            return true;
        }

        default:
            return false;
    }
}
