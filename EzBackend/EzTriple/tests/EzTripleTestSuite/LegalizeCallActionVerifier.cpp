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

    // Advance to find the CALL instruction first so we can extract the true Call Token
    // to verify against all associated PUSH_ARG / POP_RET instructions.
    auto callIt = it;
    size_t expectedPushCount = 0;

    if (!origOperands.empty())
    {
        expectedPushCount = (expectedDest != nullptr) ? (origOperands.size() - 2) : (origOperands.size() - 1);
    }

    std::advance(callIt, expectedPushCount);

    auto *callInstr = *callIt;
    EXPECT_EQ(callInstr->getOpCode(), MirInstructionOpCode::CALL) << "Failed to locate CALL boundary node.";

    // Operand 0 of the CALL is always our unique token register now
    EXPECT_TRUE(callInstr->getOperands()[0]->isOfType<MirRegister>())
            << "CALL Operand 0 must be the tracking Token Register";
    MirRegister *tokenReg = callInstr->getOperands()[0]->get<MirRegister>();

    // 1. Verify all generated PUSH_ARG instructions sequentially
    for (size_t i = 0; i < expectedPushCount; i++)
    {
        auto *instr = *it;
        EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::PUSH_ARG) << "Expected PUSH_ARG at step " << i;
        EXPECT_EQ(instr->getOperands().size(), 2) << "PUSH_ARG must possess exactly 2 operands: [Token, Value]";

        // Operand 0: The call token link
        EXPECT_EQ(instr->getOperands()[0], tokenReg) << "PUSH_ARG token link mismatch at index " << i;

        // Operand 1: The pushed argument value payload
        // We match against the original argument offset index (starting at index sitting after callee)
        size_t origArgIdx = (expectedDest != nullptr) ? (i + 2) : (i + 1);
        EXPECT_EQ(instr->getOperands()[1], origOperands[origArgIdx]) << "Pushed argument value mismatch at index " << i;
        it++;
    }

    // 2. Verify the CALL instruction properties
    EXPECT_EQ(*it, callInstr) << "Iterator tracking desynchronization encountered at CALL checkpoint.";

    if (expectedDest != nullptr)
    {
        // Operand 1 is the Callee Target Reference
        EXPECT_EQ(callInstr->getOperands()[1], origOperands[1]) << "Callee target operand mismatch on non-void CALL";
        EXPECT_EQ(callInstr->getOperands().size(), 2) << "CALL should be truncated to [Token, Callee]";
    }
    else
    {
        // Operand 1 is the Callee Target Reference (shifted right due to the void token injection)
        EXPECT_EQ(callInstr->getOperands()[1], origOperands[0]) << "Callee target operand mismatch on void CALL";
        EXPECT_EQ(callInstr->getOperands().size(), 2) << "void CALL should be truncated to [Token, Callee]";
    }

    it++;

    // 3. Verify the POP_RET instruction (only if there is a valid return value destination target)
    if (expectedDest != nullptr)
    {
        auto *popRetInstr = *it;
        EXPECT_EQ(popRetInstr->getOpCode(), MirInstructionOpCode::POP_RET)
                << "Missing POP_RET instruction directly after non-void CALL";
        EXPECT_EQ(popRetInstr->getOperands().size(), 2) << "POP_RET must possess exactly 2 operands: [Token, Dest]";

        // Operand 0: The Token register (matching the one CALL wrote to)
        EXPECT_EQ(popRetInstr->getOperands()[0], tokenReg) << "POP_RET token does not match CALL token";

        // Operand 1: The original return destination register
        EXPECT_EQ(popRetInstr->getOperands()[1], expectedDest)
                << "POP_RET does not target the original return register destination";
    }

    return *this;
}
