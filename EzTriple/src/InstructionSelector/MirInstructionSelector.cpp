#include "InstructionSelector/MirInstructionSelector.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"

bool MirInstructionSelector::selectFunction(MirBuilderContext *ctx, MirFunction *func)
{
    if (!ctx || !func) return false;

    bool allOk = true;
    for (MirBlock *block : func->getBlocks())
    {
        if (!selectBlock(ctx, block))
        {
            allOk = false;
        }
    }
    return allOk;
}

bool MirInstructionSelector::selectBlock(MirBuilderContext *ctx, MirBlock *block)
{
    if (!ctx || !block) return false;

    auto &instList = block->getInstructions();
    auto it = instList.begin();

    while (it != instList.end())
    {
        MirInstruction *inst = *it;
        auto nextIt = std::next(it);

        if (inst && inst->getTargetDesc() == nullptr)
        {
            select(ctx, inst);
        }

        it = nextIt;
    }

    return true;
}
