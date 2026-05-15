#include "MirPass/MirPassManager.h"

bool MirPassManager::run(TypedPoolLinkedList<struct MirFunction> *funcList,
                         TypedPoolLinkedList<struct MirFunction>::Iterator it,
                         struct MirPassManager *passManager)
{
    bool changed = false;
    while (it != funcList->end())
    {
        MirFunction *func = *it;

        for (auto &pass : m_passes)
        {
            TypedPoolLinkedList<MirBlock> *blockList = func->getBlocks();

            if (pass->getIterationPlace() == MirPassIterationPlace::Function)
                changed |= pass->run(funcList, it, passManager);

            changed |= run(func->getBlocks(),
                           func->getBlocks()->begin(),
                           passManager); // Run the manager inside the blocks.

            if (pass->getPassType() == MirPassType::Transform)
            {
                invalidateAllAnalyses();
            }
        }

        ++it;
    }

    return changed;
}

bool MirPassManager::run(TypedPoolLinkedList<struct MirBlock> *blockList,
                         TypedPoolLinkedList<struct MirBlock>::Iterator it,
                         struct MirPassManager *passManager)
{
    bool changed = false;
    while (it != blockList->end())
    {
        MirBlock *block = *it;
        for (auto &pass : m_passes)
        {
            if (pass->getIterationPlace() == MirPassIterationPlace::Block)
                changed |= pass->run(blockList, it, passManager);

            changed |= run(block->getInstructions(),
                           block->getInstructions()->begin(),
                           passManager); // Run the manager inside the instructions.

            if (pass->getPassType() == MirPassType::Transform)
            {
                invalidateAllAnalyses();
            }
        }
    }

    return changed;
}

bool MirPassManager::run(TypedPoolLinkedList<struct MirInstruction> *instrList,
                         TypedPoolLinkedList<struct MirInstruction>::Iterator it,
                         struct MirPassManager *passManager)
{
    bool changed = false;
    for (auto &pass : m_passes)
    {
        if (pass->getIterationPlace() == MirPassIterationPlace::Instruction)
            changed |= pass->run(instrList, it, passManager);

        if (pass->getPassType() == MirPassType::Transform)
        {
            invalidateAllAnalyses();
        }
    }

    return changed;
}