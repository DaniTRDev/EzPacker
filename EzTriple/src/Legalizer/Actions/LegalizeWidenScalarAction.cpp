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

    MirOperandFlag flag = instr->getOperandFlag(operandSlot);
    MirOperand *op = instr->getOperand(operandSlot);

    if (flag & MirOperandFlag::Write)
    {
        // 1. Def widening: create wide vreg, let instr write to it, truncate back to original
        MirRegister *wideDef = ob.buildVReg(targetType);
        ib.swapOperand(instr, wideDef, operandSlot);
        ib.changeInsertionType(InsertionType::InsertAfter);

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

    ib.build(extOp, instr->getSourceRef(), { wideUse, op });
    ib.swapOperand(instr, wideUse, operandSlot);

    return LegalizationResult::Legalized;
}

} // namespace LegalizeActions
