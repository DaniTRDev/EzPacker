#include "DefaultLegalizerActions/ExpandScalarAction.h"

ExpandScalarAction::ExpandScalarAction(MirBuilderContext *ctx, TargetDesc *target) : m_ctx(ctx), m_target(target) {}

const char *ExpandScalarAction::getName() { return "ExpandScalarAction"; }

LegalizeActionResult ExpandScalarAction::run(std::pmr::list<MirInstruction *> &instrList,
                                             std::pmr::list<struct MirInstruction *>::iterator it)
{
    MirInstruction *instr = *it;
    auto &operands = instr->getOperands();
    bool modifiedMir = false;
    bool _signed = instr->isSigned();
    MirOperandBuilder opBuilder(m_ctx);

    return { .m_executed = true, .m_succeeded = true, .m_mirChanged = modifiedMir };
}
