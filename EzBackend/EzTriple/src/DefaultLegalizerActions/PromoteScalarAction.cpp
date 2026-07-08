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

    /*
     * We use a separate builder for 'InsertAfter'. To make sure multiple truncates append
     * correctly after 'instr', we insert immediately after 'it'.
     */

    MirInstructionBuilder insertBeforeBuilder(m_ctx, instr->getOwner(), InsertionType::InsertBefore, it);
    MirInstructionBuilder insertAfterBuilder(m_ctx, instr->getOwner(), InsertionType::InsertAfter, it);

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
            // Already legal
            continue;
        }

        modifiedMir = true;

        if (operand->isOfType<MirRegister>())
        {
            MirRegister *targetReg = operand->get<MirRegister>();
            MirRegister *promotedReg = opBuilder.buildVReg(promotedType, targetReg->getName() + "_promoted");

            m_ctx->getDiagCollector()->builder(Diag_Trace, "PromoteScalarAction")
                    << std::format("Promoting {} to {}", targetReg->toString(), promotedReg->toString()).c_str()
                    << targetReg->getSourceRef();

            // Handle Input (Read / ReadWrite)
            if (constraint.flags & OperandFlag::Read)
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

            // Substitute operand in the current instruction
            operands[i] = promotedReg;

            // Handle Output (Write / ReadWrite)
            if (constraint.flags & OperandFlag::Write)
            {
                MirInteger *imm = opBuilder.buildInt(m_ctx->getTypeTable()->i8(), origType->getTotalSizeInBits());
                insertAfterBuilder.TRUNC(promotedReg, imm);
            }
        }
        else if (operand->isOfType<MirInteger>() || operand->isOfType<MirFloat>())
        {
            m_ctx->getDiagCollector()->builder(Diag_Trace, "PromoteScalarAction")
                    << std::format("Promoting {} to {}", operand->toString(), promotedType->getName()).c_str()
                    << operand->getSourceRef();

            // Immediates don't need extensions inserted, just update type tracking
            operand->setMirType(promotedType);
        }
    }

    return { .m_executed = true, .m_succeeded = true, .m_mirChanged = modifiedMir };
}
