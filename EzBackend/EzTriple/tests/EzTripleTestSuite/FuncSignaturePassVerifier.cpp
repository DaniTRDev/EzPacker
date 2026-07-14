#include "FuncSignaturePassVerifier.h"

FuncSignaturePassVerifier::FuncSignaturePassVerifier(MirBuilderContext *ctx, MirFunctionSignatureLegalizerPass *pass) :
    m_ctx(ctx), MirPassVerifier(pass)
{
}

FuncSignaturePassVerifier &FuncSignaturePassVerifier::beginFunc(MirFunction *func)
{
    EXPECT_NE(func, nullptr);
    m_targetFunc = func;

    return *this;
}

FuncSignaturePassVerifier &FuncSignaturePassVerifier::verifyArgs(std::vector<MirRegister *> originalArgs)
{
    EXPECT_NE(m_targetFunc, nullptr);

    MirBlock *block = m_targetFunc->getEntryPoint();
    auto it = block->getInstructions().begin();
    EXPECT_GE(block->getInstructions().size(), originalArgs.size());

    for (size_t i = 0; i < originalArgs.size(); i++, it++)
    {
        auto instr = *it;
        auto expected = originalArgs[i];

        MirInstructionVerifier(instr)
                .opcode(MirInstructionOpCode::POP_ARG)
                .operandCount(1)
                .operandVerifier(0)
                .verifyRegister(expected->getMirType(), expected->isVirtual(), expected->getRegId());
    }

    return *this;
}
