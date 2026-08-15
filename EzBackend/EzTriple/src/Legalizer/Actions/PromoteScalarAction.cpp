#include "Legalizer/Actions/PromoteScalarAction.h"
#include "Diagnostics/DiagnosticMessage.h"
#include "Operand/MirOperands.h"

namespace LegalizeActions
{
LegalizationResult PromoteScalar(LegalizeCtx &ctx)
{
    auto it = ctx.m_it;
    auto &promotionMap = ctx.m_promotionMap;
    MirBuilderContext *builderCtx = ctx.m_ctx;
    MirInstruction *instr = *it;
    auto &operands = instr->getOperands();
    bool _signed = instr->isSigned();
    MirOperandBuilder opBuilder(builderCtx);
    MirInstructionBuilder insertBeforeBuilder(builderCtx, instr->getOwner(), InsertionType::InsertBefore, it);

    for (size_t i = 0; i < operands.size(); i++)
    {
        MirOperand *operand = operands[i];
        MirType *origType = operand->getMirType();
        MirType *promotedType = ctx.m_targetDesc->getNearestLegalType(origType);
        OperandConstraint constraint = instr->getMetadata().m_operandFlags[i];

        if (!promotedType)
        {
            builderCtx->getDiagCollector()->builder(Diag_Trace, "PromoteScalarAction")
                    << "Unknown promotion type for operand, skipping" << operand->getSourceRef();

            continue;
        }

        if (origType->getId() == promotedType->getId())
        {
            // Already legal
            continue;
        }

        if (operand->isOfType<MirRegister>())
        {
            bool newPromotion = false;
            MirRegister *targetReg = operand->get<MirRegister>(), *promotedReg = nullptr;
            auto promotedIt = promotionMap.find(targetReg->getRegId());

            if (promotedIt != promotionMap.end())
            {
                promotedReg = promotedIt->second;
                builderCtx->getDiagCollector()->builder(Diag_Trace, "PromoteScalarAction")
                        << std::format("Reusing promotion of register '{}' to '{}'",
                                       targetReg->toString(),
                                       promotedReg->toString())
                                   .c_str()
                        << targetReg->getSourceRef();
            }
            else
            {
                builderCtx->getDiagCollector()->builder(Diag_Trace, "PromoteScalarAction")
                        << std::format("Promoting register '{}' from '{}' to '{}'",
                                       targetReg->getName(),
                                       origType->getName(),
                                       promotedType->getName())
                                   .c_str()
                        << targetReg->getSourceRef();

                promotedReg = opBuilder.buildVReg(promotedType, targetReg->getName() + "_promoted");
                promotionMap[targetReg->getRegId()] = promotedReg;
                newPromotion = true;
            }

            // Handle Input (Read / ReadWrite)
            /*
             * We must inject an extension instruction *before* the current instruction to safely widen the incoming
             * narrow data into its legal size container.
             *
             * Only insert the needed extend instructions 1 time in the very first use of the register.
             */
            if ((constraint.flags & OperandFlag::Read) && newPromotion)
            {
                if (origType->getKind() == MirTypeKind::Integer)
                {
                    if (_signed)
                        insertBeforeBuilder.SEXT(promotedReg, targetReg);
                    else
                        insertBeforeBuilder.ZEXT(promotedReg, targetReg);
                }
                else
                {
                    insertBeforeBuilder.FPEXT(promotedReg, targetReg);
                }
            }

            operands[i] = promotedReg;
        }
        else if (operand->isOfType<MirInteger>())
        {
            auto *intImm = operand->get<MirInteger>();

            builderCtx->getDiagCollector()->builder(Diag_Trace, "PromoteScalarAction")
                    << std::format("Promoting immediate integer '{}' from {} to {}",
                                   intImm->toString(),
                                   origType->getName(),
                                   promotedType->getName())
                               .c_str()
                    << operand->getSourceRef();

            // Force the underlying value to sign-extend or zero-extend to match the promotion.
            intImm->getValue().extend(promotedType->getTotalSizeInBits(), _signed);
            operand->setMirType(promotedType); // Update MIR's type to ackwnoledge type change.
        }
        else if (operand->isOfType<MirFloat>())
        {
            auto *floatImm = operand->get<MirFloat>();

            builderCtx->getDiagCollector()->builder(Diag_Trace, "PromoteScalarAction")
                    << std::format("Promoting immediate float '{}' to '{}'",
                                   floatImm->toString(),
                                   promotedType->getName())
                               .c_str()
                    << operand->getSourceRef();

            floatImm->getValue().extend(promotedType->getTotalSizeInBits());
            operand->setMirType(promotedType);
        }
    }

    return LegalizationResult::Legalized;
}
}; // namespace LegalizeActions
