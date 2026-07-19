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

LegalizeCallActionVerifier &
LegalizeCallActionVerifier::verifySretCallSequence(std::pmr::list<MirInstruction *>::iterator startIt,
                                                   const std::vector<MirOperand *> &origOperands)
{
    // The original layout configuration is expected to match: [DestReg, CalleeTarget, UserArgs...]
    EXPECT_GE(origOperands.size(), 2)
            << "SRET source signature vector needs at least a Dest and Callee target reference.";

    MirOperand *highLevelDest = origOperands[0];
    MirOperand *calleeRef = origOperands[1];

    auto it = startIt;

    // 1. Verify the temporary allocation block for the structural output footprint
    MirInstruction *allocInstr = *it;
    EXPECT_EQ(allocInstr->getOpCode(), MirInstructionOpCode::ALLOC)
            << "SRET lowering sequence must initiate by emitting a stack frame ALLOC node!";

    MirRegister *sretAllocPtr = allocInstr->getOperands()[0]->get<MirRegister>();
    EXPECT_TRUE(sretAllocPtr->getMirType()->getKind() == MirTypeKind::Pointer)
            << "The ALLOC node destination must define a valid pointer handle tracking type.";

    // 2. Advance to verify the implicit address token pointer PUSH_ARG node
    ++it;
    MirInstruction *implicitPush = *it;
    EXPECT_EQ(implicitPush->getOpCode(), MirInstructionOpCode::PUSH_ARG)
            << "Missing primary argument PUSH_ARG matching the hidden SRET reference address.";

    MirRegister *callToken = implicitPush->getOperands()[0]->get<MirRegister>();
    EXPECT_EQ(implicitPush->getOperands()[1], sretAllocPtr)
            << "The implicit PUSH_ARG must forward the register assigned by our ALLOC marker.";

    // 3. Verify user arguments are mapped to sequential PUSH_ARG instructions bound to the identical callToken
    for (size_t i = 2; i < origOperands.size(); ++i)
    {
        ++it;
        MirInstruction *userPush = *it;
        EXPECT_EQ(userPush->getOpCode(), MirInstructionOpCode::PUSH_ARG);
        EXPECT_EQ(userPush->getOperands()[0]->get<MirRegister>(), callToken)
                << "User arguments must utilize the identical binding call token.";
        EXPECT_EQ(userPush->getOperands()[1], origOperands[i])
                << "Mismatched argument payload passed into the linearized push sequence stream.";
    }

    // 4. Verify the terminal CALL instruction layout structure adjustment
    ++it;
    MirInstruction *callInstr = *it;
    EXPECT_EQ(callInstr->getOpCode(), MirInstructionOpCode::CALL);
    EXPECT_EQ(callInstr->getOperands()[0]->get<MirRegister>(), callToken)
            << "The CALL operation operand 0 must capture the grouping tracking token.";
    EXPECT_EQ(callInstr->getOperands()[1], calleeRef)
            << "Mismatched callee reference target identifier bound into structural execution.";
    EXPECT_EQ(callInstr->getOperands().size(), 2)
            << "The internalized CALL layout must contain strictly 2 tracking values post legalization pruning.";

    return *this;
}