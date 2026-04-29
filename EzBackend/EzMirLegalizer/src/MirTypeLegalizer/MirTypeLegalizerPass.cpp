#include "MirTypeLegalizer/MirTypeLegalizerPass.h"

MirTypeLegalizerPass::MirTypeLegalizerPass(MirLegalizerContext *ctx) : m_ctx(ctx) {}

bool MirTypeLegalizerPass::run(MirFunction *func, MirPassManager *passManager)
{
    bool changed = false;
    m_splitMap.clear(); // Reset state for the new function
    m_promoteMap.clear();

    for (MirBlock *block : *func->getBlocks())
    {
        auto instrList = block->getInstructions();
        auto it = instrList->begin();

        while (it)
        {
            auto nextIt = runOnInstruction(it, block);
            if (nextIt != it)
            {
                changed = true;
            }
            it = ++nextIt;
        }
    }
    return changed;
}

TypedPoolSlice<MirInstruction>::Iterator
MirTypeLegalizerPass::runOnInstruction(TypedPoolSlice<MirInstruction>::Iterator instrIt, MirBlock *parentBlock)
{
    ABIDesc *desc = m_ctx->getAbiDesc();
    bool needsExpansion = false;
    bool needsPromotion = false;
    MirInstruction *instr = *instrIt;
    size_t targetRegBytes = desc->getRegSizeInBits() / 8;
    auto *operands = instr->getOperands();

    // Detect if the instruction violates target ABI sizes
    for (MirOperand *operand : *operands)
    {
        if (operand->getType() == MirOperandType::Register)
        {
            MirRegister *reg = operand->getRegister();
            if (reg->m_sizeInBytes > targetRegBytes || m_splitMap.count(reg->m_id))
            {
                needsExpansion = true;
            }
            else if (reg->m_sizeInBytes < targetRegBytes || m_promoteMap.count(reg->m_id))
            {
                needsPromotion = true;
            }
        }
        else if (operand->getType() == MirOperandType::Integer)
        {
            if (operand->getSizeInBytes() > targetRegBytes)
            {
                needsExpansion = true;
            }
            else if (operand->getSizeInBytes() < targetRegBytes)
            {
                needsPromotion = true;
            }
        }
    }

    // A single instruction rarely needs both, but prioritize expansion
    if (needsExpansion)
    {
        MirInstructionMetadata meta = getMeta(instr->getOpCode());

        switch (meta.m_category)
        {
            case MirInstructionCategory::DataMovement:
                if (instr->getOpCode() == MirInstructionOpCode::MOV)
                    return expandMov(instrIt, parentBlock);
                break;

            case MirInstructionCategory::Memory:
                if (instr->getOpCode() == MirInstructionOpCode::STORE)
                    return expandStore(instrIt, parentBlock);
                if (instr->getOpCode() == MirInstructionOpCode::LOAD)
                    return expandLoad(instrIt, parentBlock);
                break;

            case MirInstructionCategory::Arithmetic:
                return expandArithmetic(instrIt, parentBlock);

            case MirInstructionCategory::ControlFlow:
                if (instr->getOpCode() == MirInstructionOpCode::RET)
                    return expandRet(instrIt, parentBlock);
                break;

            default:
                break; // Fallback to advancing the iterator if unhandled
        }
    }
    else if (needsPromotion)
    {
        return promoteInstruction(instrIt, parentBlock);
    }

    return instrIt;
}

SplitRegister MirTypeLegalizerPass::getOrCreateSplitRegister(const MirRegister &oldReg)
{
    auto it = m_splitMap.find(oldReg.m_id);
    if (it != m_splitMap.end())
        return it->second;

    ABIDesc *desc = m_ctx->getAbiDesc();
    MirEmitter *emitter = m_ctx->getEmitter();
    TypedPool *operandPool = m_ctx->getEmitter()->getContext()->getOperandPool();
    size_t targetRegBytes = desc->getRegSizeInBits() / 8;
    size_t numSplits = (oldReg.m_sizeInBytes + targetRegBytes - 1) / targetRegBytes;

    SplitRegister split;
    split.m_split = operandPool->createSlice<MirRegister>();

    for (size_t i = 0; i < numSplits; ++i)
    {
        MirRegister newReg = emitter->createVirtualRegister(targetRegBytes);
        operandPool->createAndAppendToSlice<MirRegister>(split.m_split, newReg);
    }

    m_splitMap[oldReg.m_id] = split;
    return split;
}

MirOperand MirTypeLegalizerPass::splitImmediate(const MirInteger *oldImm, size_t chunkIndex)
{
    size_t targetRegBytes = m_ctx->getAbiDesc()->getRegSizeInBits() / 8;
    uint64_t val = static_cast<uint64_t>(oldImm->m_value);

    val = val >> (chunkIndex * targetRegBytes * 8);
    uint64_t mask = (1ULL << (targetRegBytes * 8)) - 1;
    val = val & mask;

    return MirOperand(MirInteger{ static_cast<int64_t>(val), targetRegBytes });
}

TypedPoolSlice<MirInstruction>::Iterator
MirTypeLegalizerPass::expandLoad(TypedPoolSlice<MirInstruction>::Iterator instrIt, MirBlock *parentBlock)
{
    MirInstruction *oldInstr = *instrIt;
    MirEmitterContext *ctx = m_ctx->getEmitter()->getContext();
    auto instrList = parentBlock->getInstructions();
    TypedPool *instrPool = ctx->getInstructionPool();
    TypedPool *operandPool = ctx->getOperandPool();
    ABIDesc *desc = m_ctx->getAbiDesc();

    auto *oldOps = oldInstr->getOperands();

    // LOAD dest_reg, src_ptr, [offset_imm]
    MirOperand destOp = *(oldOps->get<MirOperand>(0));
    MirOperand ptrOp = *(oldOps->get<MirOperand>(1));

    // Safely check if your emitter included the 3rd immediate offset operand
    bool hasOffset = oldOps->m_numElems > 2;
    MirOperand offsetOp;
    if (hasOffset)
    {
        offsetOp.swapData(oldOps->get<MirOperand>(2)->getVariant());
    }

    size_t targetRegBytes = desc->getRegSizeInBits() / 8;
    SplitRegister destSplit = getOrCreateSplitRegister(*(destOp.getRegister()));
    size_t numSplits = destSplit.m_split->m_numElems;

    MirOperand currentPtr = ptrOp;

    for (size_t i = 0; i < numSplits; ++i)
    {
        // Calculate Endianness:
        // Little Endian: Lowest memory address goes to the lowest register chunk.
        // Big Endian: Lowest memory address goes to the highest register chunk.
        size_t logicalChunkIndex = desc->getEndianness() == LittleEndian ? i : (numSplits - 1 - i);
        MirRegister *destChunk = destSplit.m_split->get<MirRegister>(logicalChunkIndex);

        // If this isn't the first chunk, increment the pointer via an ADD instruction
        if (i > 0)
        {
            MirRegister nextPtrReg = m_ctx->getEmitter()->createVirtualRegister(targetRegBytes);

            auto *addOps = operandPool->createSlice<MirOperand>();
            operandPool->appendToSlice(addOps, operandPool->create<MirOperand>(nextPtrReg));
            operandPool->appendToSlice(addOps, operandPool->create<MirOperand>(currentPtr));
            operandPool->appendToSlice(
                    addOps,
                    operandPool->create<MirOperand>(MirInteger{ (int64_t)targetRegBytes, targetRegBytes }));

            MirInstruction *addOffset = instrPool->create<MirInstruction>(MirInstructionOpCode::ADD, addOps);
            instrPool->appendToSliceBefore(instrList, instrIt, addOffset);

            currentPtr.swapData(nextPtrReg); // Update pointer for current and future chunks
        }

        // Emit LOAD destChunk, currentPtr, [offsetOp]
        auto *loadOps = operandPool->createSlice<MirOperand>();
        operandPool->appendToSlice(loadOps, operandPool->create<MirOperand>(*destChunk));
        operandPool->appendToSlice(loadOps, operandPool->create<MirOperand>(currentPtr));

        if (hasOffset)
        {
            operandPool->appendToSlice(loadOps, operandPool->create<MirOperand>(offsetOp));
        }

        MirInstruction *loadInstr = instrPool->create<MirInstruction>(MirInstructionOpCode::LOAD, loadOps);
        instrPool->appendToSliceBefore(instrList, instrIt, loadInstr);
    }

    return instrPool->removeFromSlice(instrList, instrIt);
}

TypedPoolSlice<MirInstruction>::Iterator
MirTypeLegalizerPass::expandMov(TypedPoolSlice<MirInstruction>::Iterator instrIt, MirBlock *parentBlock)
{
    MirInstruction *oldInstr = *instrIt;
    MirEmitterContext *ctx = m_ctx->getEmitter()->getContext();
    auto instrList = parentBlock->getInstructions();
    TypedPool *instrPool = ctx->getInstructionPool();
    TypedPool *operandPool = ctx->getOperandPool();

    MirOperand destOp = *(oldInstr->getOperands()->get<MirOperand>(0));
    MirOperand srcOp = *(oldInstr->getOperands()->get<MirOperand>(1));

    SplitRegister destSplit = getOrCreateSplitRegister(*(destOp.getRegister()));
    size_t numSplits = destSplit.m_split->m_numElems;

    for (size_t i = 0; i < numSplits; ++i)
    {
        MirRegister *destChunk = destSplit.m_split->get<MirRegister>(i);
        MirOperand srcChunkOp;

        if (srcOp.getType() == MirOperandType::Register)
        {
            SplitRegister srcSplit = getOrCreateSplitRegister(*(srcOp.getRegister()));
            if (i < srcSplit.m_split->m_numElems)
            {
                srcChunkOp.swapData(*srcSplit.m_split->get<MirRegister>(i));
            }
            else
            {
                srcChunkOp.swapData(MirInteger{ 0, destChunk->m_sizeInBytes });
            }
        }
        else if (srcOp.getType() == MirOperandType::Integer)
        {
            srcChunkOp.swapData(splitImmediate(srcOp.getInteger(), i).getVariant());
        }

        auto *newOps = operandPool->createSlice<MirOperand>();
        operandPool->appendToSlice(newOps, operandPool->create<MirOperand>(*destChunk));
        operandPool->appendToSlice(newOps, operandPool->create<MirOperand>(srcChunkOp));

        MirInstruction *newMov = instrPool->create<MirInstruction>(MirInstructionOpCode::MOV, newOps);
        instrPool->appendToSliceBefore(instrList, instrIt, newMov);
    }

    return instrPool->removeFromSlice(instrList, instrIt);
}

TypedPoolSlice<MirInstruction>::Iterator
MirTypeLegalizerPass::expandArithmetic(TypedPoolSlice<MirInstruction>::Iterator instrIt, MirBlock *parentBlock)
{
    MirInstruction *oldInstr = *instrIt;
    MirEmitterContext *ctx = m_ctx->getEmitter()->getContext();
    auto instrList = parentBlock->getInstructions();
    TypedPool *instrPool = ctx->getInstructionPool();
    TypedPool *operandPool = ctx->getOperandPool();

    MirOperand destOp = *(oldInstr->getOperands()->get<MirOperand>(0));
    MirOperand srcOp = *(oldInstr->getOperands()->get<MirOperand>(1));

    SplitRegister destSplit = getOrCreateSplitRegister(*(destOp.getRegister()));
    size_t numSplits = destSplit.m_split->m_numElems;

    // Determine Base and Carry opcodes dynamically
    MirInstructionOpCode baseOp = oldInstr->getOpCode();
    MirInstructionOpCode carryOp = baseOp;

    if (baseOp == MirInstructionOpCode::ADD)
        carryOp = MirInstructionOpCode::ADC;
    if (baseOp == MirInstructionOpCode::SUB)
        carryOp = MirInstructionOpCode::SBB;

    for (size_t i = 0; i < numSplits; ++i)
    {
        MirRegister *destChunk = destSplit.m_split->get<MirRegister>(i);
        MirOperand srcChunkOp;

        if (srcOp.getType() == MirOperandType::Register)
        {
            SplitRegister srcSplit = getOrCreateSplitRegister(*(srcOp.getRegister()));
            if (i < srcSplit.m_split->m_numElems)
            {
                srcChunkOp.swapData(*srcSplit.m_split->get<MirRegister>(i));
            }
            else
            {
                // E.g., Adding a 32-bit register to a 64-bit register.
                // We add 0 to the upper chunk, allowing the carry flag to propagate.
                srcChunkOp.swapData(MirInteger{ 0, destChunk->m_sizeInBytes });
            }
        }
        else if (srcOp.getType() == MirOperandType::Integer)
        {
            srcChunkOp.swapData(splitImmediate(srcOp.getInteger(), i).getVariant());
        }

        auto *newOps = operandPool->createSlice<MirOperand>();
        operandPool->appendToSlice(newOps, operandPool->create<MirOperand>(*destChunk));
        operandPool->appendToSlice(newOps, operandPool->create<MirOperand>(srcChunkOp));

        // The first chunk triggers the base op (ADD/SUB). Subsequent chunks consume the carry (ADC/SBB).
        MirInstructionOpCode opToEmit = (i == 0) ? baseOp : carryOp;
        MirInstruction *newInstr = instrPool->create<MirInstruction>(opToEmit, newOps);

        instrPool->appendToSliceBefore(instrList, instrIt, newInstr);
    }

    return instrPool->removeFromSlice(instrList, instrIt);
}

TypedPoolSlice<MirInstruction>::Iterator
MirTypeLegalizerPass::expandRet(TypedPoolSlice<MirInstruction>::Iterator instrIt, MirBlock *parentBlock)
{
    MirInstruction *oldInstr = *instrIt;
    auto instrList = parentBlock->getInstructions();
    TypedPool *pool = m_ctx->getEmitter()->getContext()->getOperandPool();

    MirOperand retValOp = *(oldInstr->getOperands()->get<MirOperand>(0));

    SplitRegister valSplit;
    size_t numSplits = 0;
    size_t targetRegBytes = m_ctx->getAbiDesc()->getRegSizeInBits() / 8;

    if (retValOp.getType() == MirOperandType::Register)
    {
        valSplit = getOrCreateSplitRegister(*(retValOp.getRegister()));
        numSplits = valSplit.m_split->m_numElems;
    }
    else if (retValOp.getType() == MirOperandType::Integer)
    {
        numSplits = (retValOp.getSizeInBytes() + targetRegBytes - 1) / targetRegBytes;
    }

    auto *newOps = pool->createSlice<MirOperand>();

    // Append the legal-sized chunks into the new RET instruction
    for (size_t i = 0; i < numSplits; ++i)
    {
        // Endianness dictates which chunk goes first in the return sequence
        size_t logicalChunkIndex = m_ctx->getAbiDesc()->getEndianness() == LittleEndian ? i : (numSplits - 1 - i);

        if (retValOp.getType() == MirOperandType::Register)
        {
            MirRegister *chunkReg = valSplit.m_split->get<MirRegister>(logicalChunkIndex);
            pool->appendToSlice(newOps, pool->create<MirOperand>(*chunkReg));
        }
        else
        {
            pool->appendToSlice(newOps,
                                pool->create<MirOperand>(splitImmediate(retValOp.getInteger(), logicalChunkIndex)));
        }
    }

    MirInstruction *newRet = pool->create<MirInstruction>(MirInstructionOpCode::RET, newOps);
    pool->appendToSliceBefore(instrList, instrIt, newRet);

    return pool->removeFromSlice(instrList, instrIt);
}

TypedPoolSlice<MirInstruction>::Iterator
MirTypeLegalizerPass::expandStore(TypedPoolSlice<MirInstruction>::Iterator instrIt, MirBlock *parentBlock)
{
    ABIDesc *desc = m_ctx->getAbiDesc();
    MirInstruction *oldInstr = *instrIt;
    MirEmitterContext *ctx = m_ctx->getEmitter()->getContext();
    auto instrList = parentBlock->getInstructions();
    TypedPool *instrPool = ctx->getInstructionPool();
    TypedPool *operandPool = ctx->getOperandPool();

    MirOperand ptrOp = *(oldInstr->getOperands()->get<MirOperand>(0));
    MirOperand valOp = *(oldInstr->getOperands()->get<MirOperand>(1));

    size_t targetRegBytes = desc->getRegSizeInBits() / 8;
    size_t numSplits = 0;
    SplitRegister valSplit;

    if (valOp.getType() == MirOperandType::Register)
    {
        valSplit = getOrCreateSplitRegister(*(valOp.getRegister()));
        numSplits = valSplit.m_split->m_numElems;
    }
    else if (valOp.getType() == MirOperandType::Integer)
    {
        numSplits = (valOp.getSizeInBytes() + targetRegBytes - 1) / targetRegBytes;
    }

    MirOperand currentPtr = ptrOp;

    for (size_t i = 0; i < numSplits; ++i)
    {
        size_t logicalChunkIndex = desc->getEndianness() == LittleEndian ? i : (numSplits - 1 - i);

        MirOperand chunkValOp;
        if (valOp.getType() == MirOperandType::Register)
        {
            chunkValOp.swapData(*valSplit.m_split->get<MirRegister>(logicalChunkIndex));
        }
        else
        {
            chunkValOp.swapData(splitImmediate(valOp.getInteger(), logicalChunkIndex).getVariant());
        }

        if (i > 0)
        {
            MirRegister nextPtrReg = m_ctx->getEmitter()->createVirtualRegister(targetRegBytes);
            auto *addOps = operandPool->createSlice<MirOperand>();
            operandPool->appendToSlice(addOps, operandPool->create<MirOperand>(nextPtrReg));
            operandPool->appendToSlice(addOps, operandPool->create<MirOperand>(currentPtr));
            operandPool->appendToSlice(
                    addOps,
                    operandPool->create<MirOperand>(MirInteger{ (int64_t)targetRegBytes, targetRegBytes }));

            MirInstruction *addOffset = instrPool->create<MirInstruction>(MirInstructionOpCode::ADD, addOps);
            instrPool->appendToSliceBefore(instrList, instrIt, addOffset);
            currentPtr.swapData(nextPtrReg);
        }

        auto *storeOps = operandPool->createSlice<MirOperand>();
        operandPool->appendToSlice(storeOps, operandPool->create<MirOperand>(currentPtr));
        operandPool->appendToSlice(storeOps, operandPool->create<MirOperand>(chunkValOp));

        MirInstruction *storeInstr = instrPool->create<MirInstruction>(MirInstructionOpCode::STORE, storeOps);
        instrPool->appendToSliceBefore(instrList, instrIt, storeInstr);
    }

    return instrPool->removeFromSlice(instrList, instrIt);
}

MirRegister MirTypeLegalizerPass::getOrCreatePromotedRegister(const MirRegister &oldReg)
{
    auto it = m_promoteMap.find(oldReg.m_id);
    if (it != m_promoteMap.end())
        return it->second;

    size_t targetRegBytes = m_ctx->getAbiDesc()->getRegSizeInBits() / 8;
    MirRegister promotedReg = m_ctx->getEmitter()->createVirtualRegister(targetRegBytes);
    m_promoteMap[oldReg.m_id] = promotedReg;
    return promotedReg;
}

TypedPoolSlice<MirInstruction>::Iterator
MirTypeLegalizerPass::promoteInstruction(TypedPoolSlice<MirInstruction>::Iterator instrIt, MirBlock *parentBlock)
{
    MirInstruction *oldInstr = *instrIt;
    MirEmitterContext *ctx = m_ctx->getEmitter()->getContext();
    auto instrList = parentBlock->getInstructions();
    TypedPool *instrPool = ctx->getInstructionPool();
    TypedPool *operandPool = ctx->getOperandPool();
    size_t targetRegBytes = m_ctx->getAbiDesc()->getRegSizeInBits() / 8;

    auto *newOps = operandPool->createSlice<MirOperand>();
    size_t originalDestBytes = 0;
    MirRegister promotedDestReg;
    MirInstructionOpCode op = oldInstr->getOpCode();

    // Check if this requires an overflow wrapper
    bool isMathOperation = (op == MirInstructionOpCode::ADD || op == MirInstructionOpCode::SUB ||
                            op == MirInstructionOpCode::MUL || op == MirInstructionOpCode::IMUL);

    for (size_t i = 0; i < oldInstr->getOperands()->m_numElems; ++i)
    {
        MirOperand *operand = oldInstr->getOperands()->get<MirOperand>(i);

        if (operand->getType() == MirOperandType::Register)
        {
            MirRegister *reg = operand->getRegister();
            if (reg->m_sizeInBytes < targetRegBytes)
            {
                MirRegister promotedReg = getOrCreatePromotedRegister(*reg);
                operandPool->appendToSlice(newOps, operandPool->create<MirOperand>(promotedReg));

                if (i == 0 && isMathOperation)
                {
                    originalDestBytes = reg->m_sizeInBytes;
                    promotedDestReg = promotedReg;
                }
            }
            else
            {
                operandPool->appendToSlice(newOps, operandPool->create<MirOperand>(*operand));
            }
        }
        else if (operand->getType() == MirOperandType::Integer)
        {
            if (operand->getSizeInBytes() < targetRegBytes)
            {
                int64_t val = operand->getInteger()->m_value;
                operandPool->appendToSlice(newOps, operandPool->create<MirOperand>(MirInteger{ val, targetRegBytes }));
            }
            else
            {
                operandPool->appendToSlice(newOps, operandPool->create<MirOperand>(*operand));
            }
        }
        else
        {
            operandPool->appendToSlice(newOps, operandPool->create<MirOperand>(*operand));
        }
    }

    MirInstruction *newInstr = instrPool->create<MirInstruction>(op, newOps);
    instrPool->appendToSliceBefore(instrList, instrIt, newInstr);

    auto newInstrIt = instrIt;
    --newInstrIt;

    // Inject Overflow Mask
    if (isMathOperation && originalDestBytes > 0 && originalDestBytes < targetRegBytes)
    {
        uint64_t maskValue = (1ULL << (originalDestBytes * 8)) - 1;
        auto *maskOps = operandPool->createSlice<MirOperand>();
        operandPool->appendToSlice(maskOps, operandPool->create<MirOperand>(promotedDestReg));
        operandPool->appendToSlice(
                maskOps,
                operandPool->create<MirOperand>(MirInteger{ static_cast<int64_t>(maskValue), targetRegBytes }));

        MirInstruction *maskInstr = instrPool->create<MirInstruction>(MirInstructionOpCode::AND, maskOps);
        instrPool->appendToSliceAfter(instrList, newInstrIt, maskInstr);
    }

    return instrPool->removeFromSlice(instrList, instrIt);
}
