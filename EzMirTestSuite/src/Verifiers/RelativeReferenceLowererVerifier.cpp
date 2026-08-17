#include "Verifiers/RelativeReferenceLowererVerifier.h"

RelativeReferenceLowererVerifier &RelativeReferenceLowererVerifier::verifyInstruction(
        MirFunction *func, size_t blockId, size_t instrIndex, std::function<void(MirInstructionVerifier &)> callback)
{
    MirBlock *targetBlock = nullptr;
    for (auto *block : func->getBlocks())
    {
        if (block->getId() == blockId)
        {
            targetBlock = block;
            break;
        }
    }

    EXPECT_NE(targetBlock, nullptr) << std::format("Test Error: Block ID {} not found in function.", blockId);
    if (!targetBlock)
        return *this;

    auto &instructions = targetBlock->getInstructions();
    EXPECT_LT(instrIndex, instructions.size()) << "Test Error: Instruction index out of range.";
    if (instrIndex >= instructions.size())
        return *this;

    auto it = instructions.begin();
    std::advance(it, instrIndex);
    MirInstructionVerifier instrVerifier(*it);

    callback(instrVerifier);

    return *this;
}