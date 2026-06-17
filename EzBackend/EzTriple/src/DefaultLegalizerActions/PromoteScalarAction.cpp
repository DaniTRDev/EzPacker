#include "DefaultLegalizerActions/PromoteScalarAction.h"

PromoteScalarAction::PromoteScalarAction(MirBuilderContext *ctx, TargetDesc *target) : m_ctx(ctx), m_target(target) {}

const char *PromoteScalarAction::getName() { return "PromoteScalar"; }

LegalizeActionResult PromoteScalarAction::run(std::pmr::list<MirInstruction *> &instrList,
                                              std::pmr::list<class MirInstruction *>::iterator it)
{
    MirInstruction *instr = *it;
    auto &operands = instr->getOperands();
    bool modifiedMir = false, _signed = instr->isSigned();
    MirOperandBuilder opBuilder(m_ctx);

    for (size_t i = 0; i < operands.size(); i++)
    {
        MirOperand *operand = operands[i];
        MirType *origType = operand->getMirType();
        MirType *promotedType = m_target->getNearestLegalType(origType);
        OperandConstraint constraint = instr->getMetadata().m_operandConstraints[i];

        if (!promotedType)
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "PromoteScalarAction")
                    << "Unknown promotion type for operand" << operand->getSourceRef();

            return { .m_executed = true, .m_succeeded = false, .m_mirChanged = modifiedMir };
        }

        if (origType->getId() == promotedType->getId())
        {
            // Given operand was legal.
            continue;
        }

        auto log = m_ctx->getDiagCollector()->builder(Diag_Trace, "PromoteScalarAction");
        log << "Promoting operand" << operand->getSourceRef();
        log.appendNote(
                std::format("source type = {} -> dest type = {}", origType->getName(), promotedType->getName()).c_str(),
                nullptr);

        MirInstructionBuilder builder(m_ctx, instr->getOwner(), InsertionType::InsertBefore, it);
        MirRegister *targetReg = nullptr, *promotedReg = nullptr;

        // Scr operand (read).
        if (operand->isOfType<MirRegister>())
        {
            targetReg = operand->get<MirRegister>();
            promotedReg = opBuilder.buildVReg(promotedType, targetReg->getName() + "_promoted");

            if (origType->getKind() == MirTypeKind::Integer)
            {
                if (_signed)
                {
                    // If signed, insert a signed extension (SEXT).
                    builder.SEXT(promotedReg, targetReg);
                }
                else
                {
                    // If not signed, insert a non-signed extension (ZEXT).
                    builder.ZEXT(promotedReg, targetReg);
                }
            }
            else
            {
                // Register contains a floating point value, insert an FP ext.
                builder.FPEXT(promotedReg, targetReg);
            }

            // Update the affected register.
            operands[i] = promotedReg;
        }
        else if (operand->isOfType<MirInteger>() || operand->isOfType<MirFloat>())
        {
            // For immediate integers and floats, we just update the internal type.
            operand->setMirType(promotedType);
        }

        if (constraint.flags & OperandFlag::Write)
        {
            // Dest operand, truncate it after the execution of the instruction.
            MirInteger *truncSizeImm = opBuilder.buildInt(m_ctx->getTypeTable()->i8(), origType->getTotalSizeInBits());
            builder.setInsertionPoint(instr->getOwner(), InsertionType::InsertAfter, it);
            builder.TRUNC(promotedReg, truncSizeImm);
        }

        modifiedMir = true;
    }

    return { .m_executed = true, .m_succeeded = true, .m_mirChanged = modifiedMir };
}