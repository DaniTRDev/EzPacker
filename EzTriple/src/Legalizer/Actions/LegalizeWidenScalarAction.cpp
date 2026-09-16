#include "Legalizer/Actions/LegalizeWidenScalarAction.h"

#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetDesc.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Instruction/MirInstructionMetadata.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirType.h"
#include "Type/MirTypeTable.h"

#include <vector>

namespace LegalizeActions
{

LegalizationResult LegalizeWidenScalar(LegalizeCtx &ctx, size_t operandSlot, MirType *targetType)
{
    MirBuilderContext *builderCtx = ctx.m_ctx;
    MirInstruction *instr = *ctx.m_it;
    MirOperandBuilder ob(builderCtx);
    MirInstructionBuilder ib(builderCtx, instr->getOwner(), InsertionType::InsertBefore, ctx.m_it);

    bool isCompare = (instr->getCategory() == MirInstructionCategory::MirCat_Compare ||
                      instr->getMetadata().m_category == MirInstructionCategory::MirCat_Compare);
    if (isCompare)
    {
        MirInstructionOpCode extOp;
        if (targetType->getKind() == MirTypeKind::FloatingPoint)
            extOp = MirInstructionOpCode::FPEXT;
        else if (instr->isSigned())
            extOp = MirInstructionOpCode::SEXT;
        else
            extOp = MirInstructionOpCode::ZEXT;

        for (size_t i = 1; i < instr->getOperandCount(); ++i)
        {
            MirOperand *srcOp = instr->getOperand(i);
            if (srcOp && (instr->getOperandFlag(i) & MirOperandFlag::Read))
            {
                MirRegister *wideUse = ob.buildVReg(targetType);
                ib.setInsertionPoint(instr->getOwner(), InsertionType::InsertBefore, ctx.m_it);
                ib.build(extOp, instr->getSourceRef(), { wideUse, srcOp });
                ib.swapOperand(instr, wideUse, i);
            }
        }
        return LegalizationResult::Legalized;
    }

    MirOperandFlag flag = instr->getOperandFlag(operandSlot);
    MirOperand *op = instr->getOperand(operandSlot);

    if (flag & MirOperandFlag::Write)
    {
        MirInstructionOpCode extOp;
        if (targetType->getKind() == MirTypeKind::FloatingPoint)
            extOp = MirInstructionOpCode::FPEXT;
        else if (instr->isSigned())
            extOp = MirInstructionOpCode::SEXT;
        else
            extOp = MirInstructionOpCode::ZEXT;

        // Widen any read operands smaller than targetType before the instruction
        for (size_t i = 0; i < instr->getOperandCount(); ++i)
        {
            if (i == operandSlot)
                continue;
            MirOperand *srcOp = instr->getOperand(i);
            if (srcOp && (instr->getOperandFlag(i) & MirOperandFlag::Read))
            {
                if (srcOp->getMirType() && srcOp->getMirType()->getTotalSizeInBytes() < targetType->getTotalSizeInBytes())
                {
                    MirRegister *wideUse = ob.buildVReg(targetType);
                    ib.setInsertionPoint(instr->getOwner(), InsertionType::InsertBefore, ctx.m_it);
                    ib.build(extOp, instr->getSourceRef(), { wideUse, srcOp });
                    ib.swapOperand(instr, wideUse, i);
                }
            }
        }

        // 1. Def widening: create wide vreg, let instr write to it, truncate back to original
        MirRegister *wideDef = ob.buildVReg(targetType);
        ib.swapOperand(instr, wideDef, operandSlot);

        ib.setInsertionPoint(instr->getOwner(), InsertionType::InsertAfter, ctx.m_it);
        MirInstructionOpCode truncOp = targetType->getKind() == MirTypeKind::FloatingPoint
                ? MirInstructionOpCode::FPTRUNC
                : MirInstructionOpCode::TRUNC;
        ib.build(truncOp, instr->getSourceRef(), { op, wideDef });
        return LegalizationResult::Legalized;
    }

    // 2. Use widening: extend input to targetType before the instruction
    MirRegister *wideUse = ob.buildVReg(targetType);
    MirInstructionOpCode extOp;
    if (targetType->getKind() == MirTypeKind::FloatingPoint)
        extOp = MirInstructionOpCode::FPEXT;
    else if (instr->isSigned())
        extOp = MirInstructionOpCode::SEXT;
    else
        extOp = MirInstructionOpCode::ZEXT;

    ib.setInsertionPoint(instr->getOwner(), InsertionType::InsertBefore, ctx.m_it);
    ib.build(extOp, instr->getSourceRef(), { wideUse, op });
    ib.swapOperand(instr, wideUse, operandSlot);

    return LegalizationResult::Legalized;
}

} // namespace LegalizeActions
