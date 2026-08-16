#include "Verifiers/ExpandScalarActionVerifier.h"

ExpandScalarActionVerifier::ExpandScalarActionVerifier(MirBuilderContext *ctx, MirBlockLegalizerPass *pass) :
    m_ctx(ctx), MirPassVerifier(pass)
{
}

ExpandScalarActionVerifier &ExpandScalarActionVerifier::beginBlock(MirBlock *block)
{
    EXPECT_NE(block, nullptr);
    m_targetBlock = block;

    return *this;
}

ExpandScalarActionVerifier &
ExpandScalarActionVerifier::expectInstructionSequence(const std::vector<MirInstructionOpCode> &expectedOpcodes)
{
    EXPECT_EQ(expectedOpcodes.size(), m_targetBlock->getInstructions().size());

    for (size_t i = 0; i < expectedOpcodes.size(); i++)
    {
        MirInstruction *instr = m_targetBlock->at(i);
        EXPECT_EQ(instr->getOpCode(), expectedOpcodes[i]);
    }

    return *this;
}

ExpandScalarActionVerifier &ExpandScalarActionVerifier::verifyRuntimeCallSymbol(size_t index,
                                                                                const std::string &expectedSymbolName)
{
    EXPECT_GE(m_targetBlock->getInstructions().size(), index);

    MirInstruction *instr = m_targetBlock->at(index);
    EXPECT_NE(instr, nullptr);

    MirInstructionVerifier verifier(instr);
    verifier.opcode(MirInstructionOpCode::CALL).operandVerifier(1).verifyRuntimeSymbol(expectedSymbolName);

    return *this;
}
