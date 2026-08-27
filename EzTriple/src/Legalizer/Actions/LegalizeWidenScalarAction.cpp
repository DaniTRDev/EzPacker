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

    std::vector<MirOperand *> newOperands;
    newOperands.reserve(operands.size());

    struct WidenedDef
    {
        MirRegister *origDst;
        MirRegister *widenedDst;
    };
    std::vector<WidenedDef> widenedDefs;

    bool isCompare = (instr->getMetadata().m_category == MirInstructionCategory::MirCat_Compare);

    for (size_t i = 0; i < operands.size(); ++i)
    {
        MirOperand *op = operands[i];
        if (!op)
        {
            newOperands.push_back(nullptr);
            continue;
        }

        MirOperandFlag flag = instr->getOperandFlag(i);

        // Destination / Def operand
        if (flag & MirOperandFlag::Write)
        {
            // Compare instruction destination is boolean i1 and must not be widened/truncated
            if (isCompare)
            {
                newOperands.push_back(op);
                continue;
            }

            if (op->isOfType<MirRegister>())
            {
                MirRegister *origDst = op->get<MirRegister>();
                if (origDst->getMirType() && origDst->getMirType()->getKind() != MirTypeKind::Pointer &&
                    origDst->getMirType()->getTotalSizeInBits() < resolvedTargetType->getTotalSizeInBits())
                {
                    MirRegister *widenedDst = ob.buildVReg(resolvedTargetType);
                    newOperands.push_back(widenedDst);
                    widenedDefs.push_back({ origDst, widenedDst });
                    continue;
                }
            }

            newOperands.push_back(op);
            continue;
        }

        // Source / Use operand
        if (op->isOfType<MirRegister>())
        {
            MirRegister *reg = op->get<MirRegister>();
            MirType *regType = reg->getMirType();
            if (regType && regType->getKind() != MirTypeKind::Pointer &&
                regType->getTotalSizeInBits() < resolvedTargetType->getTotalSizeInBits())
            {
                MirRegister *widenReg = ob.buildVReg(resolvedTargetType);
                if (resolvedTargetType->getKind() == MirTypeKind::FloatingPoint ||
                    regType->getKind() == MirTypeKind::FloatingPoint)
                {
                    emitInst(MirInstructionOpCode::FPEXT, { widenReg, reg });
                }
                else if (instr->isSigned())
                {
                    emitInst(MirInstructionOpCode::SEXT, { widenReg, reg });
                }
                else
                {
                    emitInst(MirInstructionOpCode::ZEXT, { widenReg, reg });
                }
                newOperands.push_back(widenReg);
                continue;
            }
        }
        else if (op->isOfType<MirInteger>())
        {
            MirInteger *imm = op->get<MirInteger>();
            if (imm->getMirType() && imm->getMirType()->getTotalSizeInBits() < resolvedTargetType->getTotalSizeInBits())
            {
                FlexInt widenFlexImm = std::move(imm->getValue());
                widenFlexImm.extend(resolvedTargetType->getTotalSizeInBits(), true);

                MirInteger *widenImm = ob.buildInt(resolvedTargetType, std::move(widenFlexImm));
                newOperands.push_back(widenImm);
                continue;
            }
        }
        else if (op->isOfType<MirFloat>())
        {
            MirFloat *imm = op->get<MirFloat>();
            if (imm->getMirType() && imm->getMirType()->getTotalSizeInBits() < resolvedTargetType->getTotalSizeInBits())
            {
                FlexFloat widenFlexImm = std::move(imm->getValue());
                widenFlexImm.extend(resolvedTargetType->getTotalSizeInBits());

                MirFloat *widenImm = ob.buildFloat(resolvedTargetType, std::move(widenFlexImm));
                newOperands.push_back(widenImm);
                continue;
            }
        }

        newOperands.push_back(op);
    }

    // Build the widened instruction
    emitInst(instr->getOpCode(), newOperands);

    // If destination was widened, truncate back to original destination
    for (const auto &wDef : widenedDefs)
    {
        if (wDef.origDst && wDef.widenedDst)
        {
            if (wDef.origDst->getMirType() && wDef.origDst->getMirType()->getKind() == MirTypeKind::FloatingPoint)
            {
                emitInst(MirInstructionOpCode::FPTRUNC, { wDef.origDst, wDef.widenedDst });
            }
            else
            {
                emitInst(MirInstructionOpCode::TRUNC, { wDef.origDst, wDef.widenedDst });
            }
        }
    }

    ib.erase(instr);
    return LegalizationResult::Legalized;
}

} // namespace LegalizeActions
