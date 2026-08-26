#include "Legalizer/Actions/LegalizeNarrowScalarAction.h"

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

LegalizationResult LegalizeNarrowScalar(LegalizeCtx &ctx, size_t operandSlot, MirType *targetType)
{
    (void)operandSlot;
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
    if (operands.size() < 3)
    {
        return LegalizationResult::NotModified;
    }

    MirTypeTable *typeTable = ctx.m_ctx->getTypeTable();
    MirType *narrowType = targetType ? targetType : typeTable->i64();
    MirType *carryType = typeTable->i1();

    MirInstructionBuilder ib(ctx.m_ctx, instr->getOwner(), InsertionType::InsertBefore, ctx.m_it);
    MirOperandBuilder ob(ctx.m_ctx);

    MirOperand *dstOp = operands[0];
    MirOperand *lhsOp = operands[1];
    MirOperand *rhsOp = operands[2];

    if (!dstOp || !lhsOp || !rhsOp)
    {
        return LegalizationResult::Failed;
    }

    // Split lhs and rhs into low and high 64-bit chunks
    MirRegister *lhsLo = ob.buildVReg(narrowType);
    MirRegister *lhsHi = ob.buildVReg(narrowType);
    ib.build(MirInstructionOpCode::UNMERGE_VALUES, instr->getSourceRef(), { lhsLo, lhsHi, lhsOp });

    MirRegister *rhsLo = ob.buildVReg(narrowType);
    MirRegister *rhsHi = ob.buildVReg(narrowType);
    ib.build(MirInstructionOpCode::UNMERGE_VALUES, instr->getSourceRef(), { rhsLo, rhsHi, rhsOp });

    MirRegister *dstLo = ob.buildVReg(narrowType);
    MirRegister *dstHi = ob.buildVReg(narrowType);

    if (instr->getOpCode() == MirInstructionOpCode::ADD)
    {
        MirRegister *carry = ob.buildVReg(carryType);
        ib.build(MirInstructionOpCode::UADDO, instr->getSourceRef(), { dstLo, carry, lhsLo, rhsLo });
        MirRegister *carryOut = ob.buildVReg(carryType);
        ib.build(MirInstructionOpCode::UADDE, instr->getSourceRef(), { dstHi, carryOut, lhsHi, rhsHi, carry });
        ib.build(MirInstructionOpCode::MERGE_VALUES, instr->getSourceRef(), { dstOp, dstLo, dstHi });
    }
    else if (instr->getOpCode() == MirInstructionOpCode::SUB)
    {
        MirRegister *borrow = ob.buildVReg(carryType);
        ib.build(MirInstructionOpCode::USUBO, instr->getSourceRef(), { dstLo, borrow, lhsLo, rhsLo });
        MirRegister *borrowOut = ob.buildVReg(carryType);
        ib.build(MirInstructionOpCode::USUBE, instr->getSourceRef(), { dstHi, borrowOut, lhsHi, rhsHi, borrow });
        ib.build(MirInstructionOpCode::MERGE_VALUES, instr->getSourceRef(), { dstOp, dstLo, dstHi });
    }
    else
    {
        // For other instructions, unmerge and recombine
        ib.build(instr->getOpCode(), instr->getSourceRef(), { dstLo, lhsLo, rhsLo });
        ib.build(instr->getOpCode(), instr->getSourceRef(), { dstHi, lhsHi, rhsHi });
        ib.build(MirInstructionOpCode::MERGE_VALUES, instr->getSourceRef(), { dstOp, dstLo, dstHi });
    }

    instr->getOwner()->getInstructions().erase(ctx.m_it);
    return LegalizationResult::Legalized;
}

} // namespace LegalizeActions
