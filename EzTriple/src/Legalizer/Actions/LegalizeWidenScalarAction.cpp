#include "Legalizer/Actions/LegalizeWidenScalarAction.h"

#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetDesc.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirType.h"
#include "Type/MirTypeTable.h"

namespace LegalizeActions
{

LegalizationResult LegalizeWidenScalar(LegalizeCtx &ctx, size_t operandSlot, MirType *targetType)
{
    if (!ctx.m_ctx || !ctx.m_targetDesc)
    {
        return LegalizationResult::Failed;
    }

    MirInstruction *instr = *ctx.m_it;
    if (!instr)
    {
        return LegalizationResult::NotModified;
    }

    auto &operands = instr->getOperands();
    if (operands.empty())
    {
        return LegalizationResult::NotModified;
    }

    MirType *resolvedTargetType = targetType;
    if (!resolvedTargetType)
    {
        MirType *curType = operands[0] ? operands[0]->getMirType() : nullptr;
        if (curType)
        {
            resolvedTargetType = ctx.m_targetDesc->getNearestLegalType(curType);
        }
    }

    if (!resolvedTargetType)
    {
        return LegalizationResult::Failed;
    }

    MirInstructionBuilder ib(ctx.m_ctx, instr->getOwner(), InsertionType::InsertBefore, ctx.m_it);
    MirOperandBuilder ob(ctx.m_ctx);

    std::vector<MirOperand *> newOperands;
    newOperands.reserve(operands.size());

    MirRegister *origDst = nullptr;
    MirRegister *widenedDst = nullptr;

    for (size_t i = 0; i < operands.size(); ++i)
    {
        MirOperand *op = operands[i];
        if (!op)
        {
            newOperands.push_back(nullptr);
            continue;
        }

        // Check if index 0 is destination
        if (i == 0 && op->isOfType<MirRegister>() && !(instr->getFlags() & MirInstructionFlags::ReadsMemory))
        {
            origDst = op->get<MirRegister>();
            if (origDst->getMirType() && origDst->getMirType()->getTotalSizeInBits() < resolvedTargetType->getTotalSizeInBits())
            {
                widenedDst = ob.buildVReg(resolvedTargetType);
                newOperands.push_back(widenedDst);
                continue;
            }
        }

        if (op->isOfType<MirRegister>())
        {
            MirRegister *reg = op->get<MirRegister>();
            if (reg->getMirType() && reg->getMirType()->getTotalSizeInBits() < resolvedTargetType->getTotalSizeInBits())
            {
                MirRegister *widenReg = ob.buildVReg(resolvedTargetType);
                ib.build(MirInstructionOpCode::ZEXT, instr->getSourceRef(), { widenReg, reg });
                newOperands.push_back(widenReg);
                continue;
            }
        }
        else if (op->isOfType<MirInteger>())
        {
            MirInteger *imm = op->get<MirInteger>();
            MirInteger *widenImm = ob.buildInt(resolvedTargetType, imm->getValue());
            newOperands.push_back(widenImm);
            continue;
        }

        newOperands.push_back(op);
    }

    // Build the widened instruction
    MirInstruction *widenedInst = ib.build(instr->getOpCode(), instr->getSourceRef(), newOperands);

    // If destination was widened, truncate back to original destination
    if (widenedDst && origDst)
    {
        ib.build(MirInstructionOpCode::TRUNC, instr->getSourceRef(), { origDst, widenedDst });
    }

    instr->getOwner()->getInstructions().erase(ctx.m_it);
    return LegalizationResult::Legalized;
}

} // namespace LegalizeActions
