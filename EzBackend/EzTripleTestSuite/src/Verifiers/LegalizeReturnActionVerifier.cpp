#include "../../include/Verifiers/LegalizeReturnActionVerifier.h"

LegalizeReturnActionVerifier::LegalizeReturnActionVerifier(MirBuilderContext *ctx, MirBlockLegalizerPass *pass) :
    m_ctx(ctx), MirPassVerifier(pass)
{
}

LegalizeReturnActionVerifier &
LegalizeReturnActionVerifier::verifyRetPush(std::pmr::list<MirInstruction *>::iterator retStartIt,
                                            MirOperand *origOperand)
{
    auto it = retStartIt;

    if (origOperand)
    {
        MirFunction *func = (*it)->getOwner()->getOwner();
        CallingConvDesc *cc = func->getCallingConv();
        bool isSret = cc && !cc->canReturnInRegs(origOperand->getMirType());

        if (isSret)
        {
            // Verify STORE Instruction: STORE ptr [sretPtr + 0], origOperand
            auto *storeInstr = *it;
            MirInstructionVerifier(storeInstr).opcode(MirInstructionOpCode::STORE).operandCount(2);

            // STORE Destination: Memory operand pointing to the function's first parameter (sretPtr)
            EXPECT_FALSE(func->getParameters().empty()) << "SRET return expects an implicit sret parameter.";
            MirRegister *sretPtr = func->getParameters().front();

            EXPECT_TRUE(storeInstr->getOperands()[0]->isOfType<MirMemory>())
                    << "STORE operand 0 must be a memory operand.";
            auto *memOp = storeInstr->getOperands()[0]->get<MirMemory>();
            EXPECT_EQ(memOp->getBase()->getRegId(), sretPtr->getRegId())
                    << "STORE memory base register must match the implicit SRET pointer parameter.";

            // STORE Value: The original return payload value
            EXPECT_EQ(storeInstr->getOperands()[1], origOperand)
                    << "STORE payload value does not match the original return operand.";

            it++; // Advance to PUSH_RET
        }

        auto *pushRetInstr = *it;
        MirInstructionVerifier(pushRetInstr).opcode(MirInstructionOpCode::PUSH_RET).operandCount(2);

        // Operand 0: Token Register injected by the legalizer
        EXPECT_TRUE(pushRetInstr->getOperands()[0]->isOfType<MirRegister>())
                << "PUSH_RET Operand 0 must be a token register.";
        MirRegister *tokenReg = pushRetInstr->getOperands()[0]->get<MirRegister>();
        EXPECT_EQ(tokenReg->getMirType()->getKind(), MirTypeKind::BindingToken);

        // Operand 1: SRET returns carry the sretPtr; direct returns carry origOperand
        if (isSret)
        {
            MirRegister *sretPtr = func->getParameters().front();
            EXPECT_EQ(pushRetInstr->getOperands()[1], sretPtr)
                    << "PUSH_RET payload for SRET return must carry the SRET pointer parameter.";
        }
        else
        {
            EXPECT_EQ(pushRetInstr->getOperands()[1], origOperand)
                    << "PUSH_RET payload does not match the original return operand.";
        }

        it++; // Advance to RET
        auto *retInstr = *it;
        MirInstructionVerifier(retInstr).opcode(MirInstructionOpCode::RET).operandCount(1);

        // Verify that RET holds onto the exact same tracking token register
        EXPECT_EQ(retInstr->getOperands()[0], tokenReg)
                << "RET tracking token does not match the token bound inside PUSH_RET.";
    }
    else
    {
        auto *retInstr = *it;
        MirInstructionVerifier(retInstr).opcode(MirInstructionOpCode::RET).operandCount(1);

        // Verify that void RET is standardized to hold a token operand
        EXPECT_TRUE(retInstr->getOperands()[0]->isOfType<MirRegister>())
                << "Void RET must contain a token register operand.";
        MirRegister *tokenReg = retInstr->getOperands()[0]->get<MirRegister>();
        EXPECT_EQ(tokenReg->getMirType()->getKind(), MirTypeKind::BindingToken);
    }

    return *this;
}