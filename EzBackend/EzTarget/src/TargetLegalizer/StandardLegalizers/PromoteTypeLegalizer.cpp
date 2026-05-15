#include "TargetDesc.h"
#include "TargetLegalizer/StandardLegalizers/PromoteTypeLegalizer.h"

bool StandardLegalizers::promoteTypeLegalizer(MirEmitter *emitter,
                                              TargetDesc *targetDesc,
                                              TypedPoolLinkedList<class MirInstruction>::Iterator it,
                                              TypedPoolLinkedList<class MirOperand>::Iterator operand,
                                              size_t operandId)
{
    ABIDesc *abi = targetDesc->getABI();
    MirEmitterContext *emitterCtx = emitter->getContext();
    MirBlock *currentBlock = emitterCtx->getCurrentBoundBlock();

    MirInstruction *instr = *it;
    MirOperand *op = *operand;

    size_t legalSizeInBytes = abi->getRegSizeInBits() / 8;
    bool modified = false;

    // Skip operands that are already legal or don't need to be promoted.
    if (op->getSizeInBytes() >= legalSizeInBytes)
    {
        return false;
    }
    
    MirType *promotedType = emitterCtx->getIntegerTypeBySize(legalSizeInBytes);
    if (op->isOfType<MirRegister>())
    {
        MirRegister *origReg = op->get<MirRegister>();
        MirRegister *promotedReg = emitter->createVirtualRegister(promotedType);

        // Fetch the metadata to check if this operand is an input (Read) or output (Write)
        const MirInstructionMetadata &meta = getMeta(instr->getOpCode());
        const OperandConstraint &constraint = meta.m_operandConstraints[operandId];

        // Are we promoting an Output (Def) or an Input (Use)?
        if (constraint.flags & OperandFlag::Write)
        {
            // Swap the instruction's destination to the new, large register
            operand.m_curr->m_object = promotedReg;

            // 2. Emit a Truncate AFTER this instruction => origReg = TRUNC promotedReg
            auto insertAfterIt = it;
            ++insertAfterIt; // Point to the next instruction

            emitterCtx->setInsertPoint(currentBlock, insertAfterIt);
            emitter->emit(MirInstructionOpCode::TRUNC, { origReg, promotedReg });
        }
        else // Promoting an Input (Use)
        {
            bool isSigned = meta.m_flags & MirInstructionFlags::TreatAsSigned;
            MirInstructionOpCode extOpcode = isSigned ? MirInstructionOpCode::SEXT : MirInstructionOpCode::ZEXT;

            // Emit the extension BEFORE this instruction: promotedReg = EXT origReg
            emitterCtx->setInsertPoint(currentBlock, it);
            emitter->emit(extOpcode, { promotedReg, origReg });

            // Swap the instruction's source to use the newly extended register
            operand.m_curr->m_object = promotedReg;
        }

        // Restore the emitter to normal append mode
        emitterCtx->setInsertPoint(currentBlock);

        modified = true;
    }
    else if (op->isOfType<MirInteger>())
    {
        MirInteger *integer = op->get<MirInteger>();

        const MirInstructionMetadata &meta = getMeta(instr->getOpCode());
        bool isSigned = meta.m_flags & MirInstructionFlags::TreatAsSigned;

        int64_t rawValue = integer->getValue();
        size_t oldSize = integer->getSizeInBytes();

        // If it's strictly unsigned, we must mask off the upper bits to prevent
        // C++ from accidentally sign-extending negative-looking values.
        if (!isSigned && oldSize < 8)
        {
            uint64_t mask = (1ULL << (oldSize * 8)) - 1;
            rawValue = static_cast<int64_t>(static_cast<uint64_t>(rawValue) & mask);
        }

        // Replace the old immediate with the promoted one
        operand.m_curr->m_object = emitter->createImmediateInteger(promotedType, rawValue);

        modified = true;
    }
    else if (op->isOfType<MirDouble>())
    {
        // TODO: After adding FPU support.
    }

    return modified;
}
