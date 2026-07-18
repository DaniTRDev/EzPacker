#include "DefaultLegalizerActions/LegalizeReturnAction.h"

LegalizeReturnAction::LegalizeReturnAction(MirBuilderContext *ctx) : m_ctx(ctx) {}

const char *LegalizeReturnAction::getName() { return "LegalizeReturnAction"; }

LegalizeActionResult LegalizeReturnAction::run(std::pmr::list<MirInstruction *> &instrList,
                                               std::pmr::list<struct MirInstruction *>::iterator it)
{
    MirInstruction *instr = *it;
    auto &operands = instr->getOperands();

    MirInstructionBuilder insertBeforeBuilder(m_ctx, instr->getOwner(), InsertionType::InsertBefore, it);
    MirOperandBuilder opBuilder(m_ctx);

    // Create a unique return tracking token (virtual register)
    MirRegister *retToken = opBuilder.buildVReg(m_ctx->getTypeTable()->getBindingToken());

    // For non-void functions.
    if (!operands.empty())
    {
        // Original return value being passed out of the function
        MirOperand *returnVal = operands[0];

        // PUSH_RET now explicitly groups: (sourceRef, retToken, returnVal)
        insertBeforeBuilder.PUSH_RET(instr->getSourceRef(), retToken, returnVal);

        // Instead of clearing the operands entirely, modify the RET instruction
        // to store the tracking token. A fully legalized RET now looks like: [retToken]
        operands[0] = retToken;
    }
    else
    {
        operands.push_back(retToken);
    }

    return { .m_executed = true, .m_succeeded = true, .m_mirChanged = true };
}
