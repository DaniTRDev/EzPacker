#include "DefaultLegalizerActions/LegalizeReturnAction.h"

LegalizeReturnAction::LegalizeReturnAction(MirBuilderContext *ctx) : m_ctx(ctx) {}

const char *LegalizeReturnAction::getName() { return "LegalizeReturnAction"; }

LegalizeActionResult LegalizeReturnAction::run(std::pmr::list<MirInstruction *> &instrList,
                                               std::pmr::list<struct MirInstruction *>::iterator it)
{
    MirInstruction *instr = *it;
    auto &operands = instr->getOperands();
    MirInstructionBuilder insertBeforeBuilder(m_ctx, instr->getOwner(), InsertionType::InsertBefore, it);

    insertBeforeBuilder.PUSH_RET(instr->getSourceRef(), operands[0]);
    operands.clear(); // Make a no-operand return.

    return { .m_executed = true, .m_succeeded = true, .m_mirChanged = true };
}
