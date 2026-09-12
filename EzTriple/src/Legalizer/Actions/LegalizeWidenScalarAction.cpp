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
    MirOperandBuilder ob(m_ctx);
    MirInstructionBuilder ib(m_ctx, instr->getOwner(), InsertionType::InsertBefore, instr);

    MirOperandFlag flag = instr->getOperandFlag(typeIdx);
    MirOperand *op = instr->getOperand(typeIdx);

    if (flag & MirOperandFlag::Write)
    {
        // 1. Def widening: create wide vreg, let instr write to it, truncate back to original
        MirRegister *wideDef = ob.buildVReg(targetType);
        instr->setOperand(typeIdx, wideDef);

        ib.setInsertionPoint(InsertionType::InsertAfter, instr);
        MirInstructionOpCode truncOp = targetType->isFloat() ? MirInstructionOpCode::FPTRUNC
                                                             : MirInstructionOpCode::TRUNC;
        ib.build(truncOp, instr->getSourceRef(), { op, wideDef });
        return LegalizationResult::Legalized;
    }

    // 2. Use widening: extend input to targetType before the instruction
    MirRegister *wideUse = ob.buildVReg(targetType);
    MirInstructionOpCode extOp;
    if (targetType->isFloat())      extOp = MirInstructionOpCode::FPEXT;
    else if (instr->isSigned())     extOp = MirInstructionOpCode::SEXT;
    else                            extOp = MirInstructionOpCode::ZEXT;

    ib.build(extOp, instr->getSourceRef(), { wideUse, op });
    instr->setOperand(typeIdx, wideUse);

    return LegalizationResult::Legalized;
}

} // namespace LegalizeActions
