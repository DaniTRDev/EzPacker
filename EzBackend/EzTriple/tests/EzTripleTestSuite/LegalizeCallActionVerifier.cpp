#include "LegalizeCallActionVerifier.h"
LegalizeCallActionVerifier::LegalizeCallActionVerifier(MirBuilderContext *ctx, MirBlockLegalizerPass *pass) :
    m_ctx(ctx), MirPassVerifier(pass)
{
}

LegalizeCallActionVerifier &
LegalizeCallActionVerifier::verifyCallSequence(std::pmr::list<MirInstruction *>::iterator startIt,
                                               const std::vector<MirOperand *> &origOperands,
                                               MirOperand *expectedDest)
{
    auto it = startIt;

    // Verify all generated PUSH_ARG instructions sequentially
    for (size_t i = 0; i < origOperands.size(); i++)
    {
        auto *instr = *it;
        EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::PUSH_ARG) << "Expected PUSH_ARG at step " << i;

        // Operand 0: The pushed argument value
        EXPECT_EQ(instr->getOperands()[0], origOperands[i]) << "Pushed argument mismatch at index " << i;
        it++;
    }

    // Verify the CALL instruction itself
    auto *callInstr = *it;
    EXPECT_EQ(callInstr->getOpCode(), MirInstructionOpCode::CALL);

    MirRegister *tokenReg = nullptr;
    if (expectedDest != nullptr)
    {
        // If there was a destination, Operand 0 MUST be the newly created Token Register
        EXPECT_TRUE(callInstr->getOperands()[0]->isOfType<MirRegister>());
        tokenReg = callInstr->getOperands()[0]->get<MirRegister>();

        // Operand 1 is the Callee Target Reference
        EXPECT_EQ(callInstr->getOperands().size(), 2) << "CALL should be truncated to [Token, Callee]";
    }
    else
    {
        // If void, Operand 0 is the Callee Target Reference directly
        EXPECT_EQ(callInstr->getOperands().size(), 1) << "CALL should be truncated to [Callee]";
    }

    it++;

    // Verify the POP_RET instruction (only if there is a return destination)
    if (expectedDest != nullptr)
    {
        auto *popRetInstr = *it;
        EXPECT_EQ(popRetInstr->getOpCode(), MirInstructionOpCode::POP_RET)
                << "Missing POP_RET instruction directly after CALL";

        // Operand 0: The Token register (matching the one CALL wrote to)
        EXPECT_EQ(popRetInstr->getOperands()[0], tokenReg) << "POP_RET token does not match CALL token";

        // Operand 1: The original return destination register
        EXPECT_EQ(popRetInstr->getOperands()[1], expectedDest)
                << "POP_RET does not target the original return register destination";
    }

    return *this;
}
