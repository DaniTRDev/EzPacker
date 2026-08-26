#include "Legalizer/Actions/LegalizeBitcastAction.h"

#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetDesc.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirType.h"

namespace LegalizeActions
{

LegalizationResult LegalizeBitcast(LegalizeCtx &ctx, size_t operandSlot, MirType *targetType)
{
    if (!ctx.m_ctx || !targetType)
    {
        return LegalizationResult::Failed;
    }

    MirInstruction *instr = *ctx.m_it;
    if (!instr || instr->getOperands().empty())
    {
        return LegalizationResult::NotModified;
    }

    MirInstructionBuilder ib(ctx.m_ctx, instr->getOwner(), InsertionType::InsertBefore, ctx.m_it);
    MirOperandBuilder ob(ctx.m_ctx);

    std::vector<MirOperand *> newOperands;
    for (MirOperand *op : instr->getOperands())
    {
        if (op && op->getMirType() && op->getMirType() != targetType)
        {
            MirRegister *castReg = ob.buildVReg(targetType);
            ib.build(MirInstructionOpCode::BITCAST, instr->getSourceRef(), { castReg, op });
            newOperands.push_back(castReg);
        }
        else
        {
            newOperands.push_back(op);
        }
    }

    ib.build(instr->getOpCode(), instr->getSourceRef(), newOperands);
    instr->getOwner()->getInstructions().erase(ctx.m_it);
    return LegalizationResult::Legalized;
}

} // namespace LegalizeActions
