#include "Verifiers/InstructionSelectorPassVerifier.h"

InstructionSelectorPassVerifier::InstructionSelectorPassVerifier(MirBuilderContext *ctx,
                                                                 MirInstructionSelectorPass *pass) :
    m_ctx(ctx), MirPassVerifier(pass)
{
}

InstructionSelectorPassVerifier &InstructionSelectorPassVerifier::verifyInstructionSelected(
        MirBlock *targetBlock, size_t instructionIndex, MirTargetInstructionId expectedTargetId)
{
    EXPECT_NE(targetBlock, nullptr) << "Target block must not be null.";
    auto &instructions = targetBlock->getInstructions();
    EXPECT_LT(instructionIndex, instructions.size())
            << "Instruction index " << instructionIndex << " out of bounds for block " << targetBlock->getName()
            << " (size: " << instructions.size() << ").";

    auto it = instructions.begin();
    std::advance(it, instructionIndex);
    MirInstruction *instr = *it;

    EXPECT_NE(instr, nullptr) << "Instruction at index " << instructionIndex << " must not be null.";
    EXPECT_NE(instr->getTargetId(), MIRID_INVALID) << "Instruction at index " << instructionIndex << " ("
                                                   << instr->getOpCodeName() << ") does not have a target ID assigned.";
    EXPECT_EQ(instr->getTargetId(), expectedTargetId)
            << "Target ID mismatch for instruction at index " << instructionIndex << " (" << instr->getOpCodeName()
            << "). Expected: " << expectedTargetId << ", Actual: " << instr->getTargetId();

    return *this;
}
