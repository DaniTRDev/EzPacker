#include "Legalizer/Actions/LegalizeBitcastAction.h"

#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetDesc.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Instruction/MirInstructionMetadata.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirType.h"

#include <vector>

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

    auto &operands = instr->getOperands();
    MirInstructionBuilder ib(ctx.m_ctx, instr->getOwner(), InsertionType::InsertBefore, ctx.m_it);
    MirOperandBuilder ob(ctx.m_ctx);

    bool firstInserted = false;
    auto emitInst = [&](MirInstructionOpCode opc, const std::vector<MirOperand *> &ops)
    {
        ib.build(opc, instr->getSourceRef(), ops);
        if (!firstInserted)
        {
            ib.changeInsertionType(InsertionType::InsertAfter);
            firstInserted = true;
        }
    };

    std::vector<MirOperand *> newOperands(operands.begin(), operands.end());
    struct BitcastDef
    {
        MirRegister *origDst;
        MirRegister *tempDst;
    };
    std::vector<BitcastDef> bitcastDefs;

    auto processSlot = [&](size_t slot)
    {
        if (slot >= operands.size() || !operands[slot])
            return;
            
        MirOperand *op = operands[slot];
        if (!op->getMirType() || op->getMirType() == targetType)
            return;

        MirOperandFlag flag = instr->getOperandFlag(slot);
        if (flag & MirOperandFlag::Write)
        {
            if (op->isOfType<MirRegister>())
            {
                MirRegister *origDst = op->get<MirRegister>();
                MirRegister *tempDst = ob.buildVReg(targetType);
                newOperands[slot] = tempDst;
                bitcastDefs.push_back({ origDst, tempDst });
            }
        }
        else
        {
            MirRegister *castReg = ob.buildVReg(targetType);
            emitInst(MirInstructionOpCode::BITCAST, { castReg, op });
            newOperands[slot] = castReg;
        }
    };

    if (operandSlot < operands.size())
    {
        processSlot(operandSlot);
    }
    else
    {
        for (size_t i = 0; i < operands.size(); ++i)
        {
            processSlot(i);
        }
    }

    emitInst(instr->getOpCode(), newOperands);

    for (const auto &bDef : bitcastDefs)
    {
        emitInst(MirInstructionOpCode::BITCAST, { bDef.origDst, bDef.tempDst });
    }

    instr->getOwner()->getInstructions().erase(ctx.m_it);
    return LegalizationResult::Legalized;
}

} // namespace LegalizeActions
