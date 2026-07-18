#include "DefaultLegalizerActions/LegalizeCallAction.h"

LegalizeCallAction::LegalizeCallAction(MirBuilderContext *ctx) : m_ctx(ctx) {}

const char *LegalizeCallAction::getName() { return "LegalizeCallAction"; }

LegalizeActionResult LegalizeCallAction::run(std::pmr::list<MirInstruction *> &instrList,
                                             std::pmr::list<struct MirInstruction *>::iterator it)
{
    MirInstruction *instr = *it;
    auto &operands = instr->getOperands();
    bool modifiedMir = false;

    MirInstructionBuilder builder(m_ctx, instr->getOwner(), InsertionType::InsertBefore, it);
    MirOperandBuilder opBuilder(m_ctx);

    /*
     * First operand = return place / destination.
     */
    MirOperand *returnDest = operands[0];

    // Every call now gets a unique call token to group its argument/return lifecycle.
    MirRegister *callToken = opBuilder.buildVReg(m_ctx->getTypeTable()->getBindingToken());

    if (returnDest->isOfType<MirRegister>())
    {
        // Insert POP_RET AFTER THE CALL, structurally tied to this call's token.
        builder.changeInsertionType(InsertionType::InsertAfter);
        builder.POP_RET(instr->getSourceRef(), callToken, returnDest);
        builder.changeInsertionType(InsertionType::InsertBefore);

        // Swap the target return destination inside the CALL with our tracking token.
        operands[0] = callToken;
        modifiedMir = true;
    }
    else
    {
        // For void functions or direct callee links, insert the callToken as an explicit operand at the beginning of
        // the CALL's operand storage array.
        operands.insert(operands.begin(), callToken);
        modifiedMir = true;
    }

    // Since we've standardized the layout, the callee reference sits at index 1,
    // and the high-level call arguments begin at index 2.
    size_t startingId = 2;

    // Insert PUSH_ARG instructions BEFORE THE CALL, each referencing the tracking token.
    for (size_t i = startingId; i < operands.size(); ++i)
    {
        // PUSH_ARG syntax updated to accept: (sourceRef, callToken, argValue)
        builder.PUSH_ARG(instr->getSourceRef(), callToken, operands[i]);
        modifiedMir = true;
    }

    // Clear the high-level parameter list out of the core CALL instruction.
    // It now only tracks: [callToken, calleeTarget]
    operands.erase(operands.begin() + int64_t(startingId), operands.end());
    return { .m_executed = true, .m_succeeded = true, .m_mirChanged = modifiedMir };
}