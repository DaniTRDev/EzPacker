#include "DefaultLegalizerActions/PromoteScalarAction.h"
#include "Diagnostics/DiagnosticMessage.h"
#include "Operand/MirOperands.h"

PromoteScalarAction::PromoteScalarAction(MirBuilderContext *ctx, TargetDesc *target) : m_ctx(ctx), m_target(target) {}

const char *PromoteScalarAction::getName() { return "PromoteScalar"; }

LegalizeActionResult PromoteScalarAction::run(std::pmr::list<MirInstruction *> &instrList,
                                              std::pmr::list<class MirInstruction *>::iterator it)
{
    MirInstruction *instr = *it;
    auto &operands = instr->getOperands();
    bool modifiedMir = false;
    bool _signed = instr->isSigned();
    MirOperandBuilder opBuilder(m_ctx);
    MirInstructionBuilder insertBeforeBuilder(m_ctx, instr->getOwner(), InsertionType::InsertBefore, it);

    for (size_t i = 0; i < operands.size(); i++)
    {
        MirOperand *operand = operands[i];
        MirType *origType = operand->getMirType();
        MirType *promotedType = m_target->getNearestLegalType(origType);
        OperandConstraint constraint = instr->getMetadata().m_operandConstraints[i];

        if (!promotedType)
        {
            m_ctx->getDiagCollector()->builder(Diag_Trace, "PromoteScalarAction")
                    << "Unknown promotion type for operand, skipping" << operand->getSourceRef();

            continue;
        }

        if (origType->getId() == promotedType->getId())
        {
            // Already legal
            continue;
        }

        modifiedMir = true;

        if (operand->isOfType<MirRegister>())
        {
            bool newPromotion = false;
            MirRegister *targetReg = operand->get<MirRegister>(), *promotedReg = nullptr;
            auto promotedIt = m_promotionMap.find(targetReg->getRegId());

            if (promotedIt != m_promotionMap.end())
            {
                promotedReg = promotedIt->second;
                m_ctx->getDiagCollector()->builder(Diag_Trace, "PromoteScalarAction")
                        << std::format("Reusing promotion of register '{}' to '{}'",
                                       targetReg->toString(),
                                       promotedReg->toString())
                                   .c_str()
                        << targetReg->getSourceRef();
            }
            else
            {
                m_ctx->getDiagCollector()->builder(Diag_Trace, "PromoteScalarAction")
                        << std::format("Promoting register '{}' from '{}' to '{}'",
                                       targetReg->getName(),
                                       origType->getName(),
                                       promotedType->getName())
                                   .c_str()
                        << targetReg->getSourceRef();

                promotedReg = opBuilder.buildVReg(promotedType, targetReg->getName() + "_promoted");
                m_promotionMap[targetReg->getRegId()] = promotedReg;
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

            m_ctx->getDiagCollector()->builder(Diag_Trace, "PromoteScalarAction")
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

            m_ctx->getDiagCollector()->builder(Diag_Trace, "PromoteScalarAction")
                    << std::format("Promoting immediate float '{}' to '{}'",
                                   floatImm->toString(),
                                   promotedType->getName())
                               .c_str()
                    << operand->getSourceRef();

            floatImm->getValue().extend(promotedType->getTotalSizeInBits());
            operand->setMirType(promotedType);
        }
    }

    return { .m_executed = true, .m_succeeded = true, .m_mirChanged = modifiedMir };
}
