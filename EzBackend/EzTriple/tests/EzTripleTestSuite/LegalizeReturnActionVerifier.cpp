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
        auto pushRetInstr = *retStartIt;
        auto verifier = MirInstructionVerifier(pushRetInstr)
                                .opcode(MirInstructionOpCode::PUSH_RET)
                                .operandCount(origOperand ? 1 : 0);

        verifier.operandVerifier(0).type(origOperand->getType());
        // No operand was created/copied, only MOVED which makes the pointer address be the same.
        EXPECT_TRUE(pushRetInstr->getOperands()[0] == origOperand);

        auto retInstr = *(++retStartIt);
        MirInstructionVerifier(retInstr).opcode(MirInstructionOpCode::RET);
    }
    else
    {
        auto retInstr = *retStartIt;
        MirInstructionVerifier(retInstr).opcode(MirInstructionOpCode::RET);
    }

    return *this;
}
