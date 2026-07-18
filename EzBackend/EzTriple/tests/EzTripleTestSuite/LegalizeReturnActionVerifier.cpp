#include "LegalizeReturnActionVerifier.h"

LegalizeReturnActionVerifier::LegalizeReturnActionVerifier(MirBuilderContext *ctx, MirBlockLegalizerPass *pass) :
    m_ctx(ctx), MirPassVerifier(pass)
{
}

LegalizeReturnActionVerifier &
LegalizeReturnActionVerifier::verifyRetPush(std::pmr::list<MirInstruction *>::iterator retStartIt,
                                            MirOperand *origOperand)
{
    if (origOperand)
    {
        auto it = retStartIt;
        auto *pushRetInstr = *it;

        // Verify PUSH_RET properties: [Token, Value]
        auto pushVerifier = MirInstructionVerifier(pushRetInstr).opcode(MirInstructionOpCode::PUSH_RET).operandCount(2);

        // Operand 0: Token Register injected by the action
        EXPECT_TRUE(pushRetInstr->getOperands()[0]->isOfType<MirRegister>())
                << "PUSH_RET Operand 0 must be a token register.";
        MirRegister *tokenReg = pushRetInstr->getOperands()[0]->get<MirRegister>();
        EXPECT_EQ(tokenReg->getMirType()->getKind(), MirTypeKind::BindingToken);

        // Operand 1: The original return payload value
        EXPECT_EQ(pushRetInstr->getOperands()[1], origOperand)
                << "PUSH_RET payload does not match the original return operand.";

        // Advance to the RET instruction
        it++;
        auto *retInstr = *it;

        auto retVerifier = MirInstructionVerifier(retInstr).opcode(MirInstructionOpCode::RET).operandCount(1);

        // Verify that RET holds onto the exact same tracking token register
        EXPECT_EQ(retInstr->getOperands()[0], tokenReg)
                << "RET tracking token does not match the token bound inside PUSH_RET.";
    }
    else
    {
        // Void returns remain unmodified with 0 operands
        auto *retInstr = *retStartIt;
        MirInstructionVerifier(retInstr).opcode(MirInstructionOpCode::RET).operandCount(0);
    }

    return *this;
}