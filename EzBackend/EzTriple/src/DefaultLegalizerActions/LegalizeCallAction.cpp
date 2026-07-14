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
     * First operand = return place. It MUST be a REGISTER, if it's not a register but it's a reference to the callee,
     * the function was a void type.
     */
    MirOperand *returnDest = operands[0];
    MirRegister *callToken = nullptr;

    if (returnDest->isOfType<MirRegister>())
    {
        // Call has a return location. Swap it with an unique token (virtual reg) to bind the call to a POP_RET.
        callToken = opBuilder.buildVReg(m_ctx->getTypeTable()->i64());

        // Now insert a POP_RET AFTER THE CALL.
        builder.changeInsertionType(InsertionType::InsertAfter);
        builder.POP_RET(instr->getSourceRef(), callToken, operands[0]);
        builder.changeInsertionType(InsertionType::InsertBefore);

        // Swap the operand to be the call token.
        operands[0] = callToken;
        modifiedMir = true;
    }

    // If a callToken is bind, we need to start at index 2 (token, callee, args)
    size_t startingId = callToken ? 2 : 1;
    for (size_t i = startingId; i < operands.size(); ++i)
    {
        builder.PUSH_ARG(instr->getSourceRef(), operands[i]);
        modifiedMir = true;
    }

    operands.erase(operands.begin() + int64_t(startingId), operands.end());
    return { .m_executed = true, .m_succeeded = true, .m_mirChanged = modifiedMir };
}
