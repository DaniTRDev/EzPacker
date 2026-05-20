#include "MirPass/MirPassManager.h"

bool MirPassManager::run(TypedPoolLinkedList<struct MirFunction> *funcList,
                         TypedPoolLinkedList<struct MirFunction>::Iterator it,
                         struct MirPassManager *passManager)
{
    bool changed = false;
    MirFunction *func = *it;

    // Run all Function-level passes on the current function
    for (auto &pass : m_passes)
    {
        if (pass->getIterationPlace() == MirPassIterationPlace::Function)
        {
            bool result = false;
            LOG_DEBUG(std::format("{:#^50} ({})", "Running function pass", pass->getName()), "MirPassManager");
            {
                LOG_DEBUG(MirPrinter().printToString(func), "MirPassManager");
                result = pass->run(funcList, it, passManager);

                LOG_DEBUG(std::format("{:#^50} (result: {})", "Pass result", result), "MirPassManager");
                LOG_DEBUG(MirPrinter().printToString(func), "MirPassManager");
            }
            LOG_DEBUG(std::format("{:#^50}", "Ran pass"), "MirPassManager");

            changed |= result;
            if (pass->getPassType() == MirPassType::Transform)
            {
                invalidateAllAnalyses();
            }
        }
    }

    // Cascade down into the blocks ONCE per function
    changed |= run(func->getBlocks(), func->getBlocks()->begin(), passManager);
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

        // Run all Block-level passes on the current block
        for (auto &pass : m_passes)
        {
            if (pass->getIterationPlace() == MirPassIterationPlace::Block)
            {
                LOG_DEBUG(std::format("{:#^50} ({})", "Running block pass", pass->getName()), "MirPassManager");
                LOG_DEBUG(MirPrinter().printToString(block), "MirPassManager");

                bool result = pass->run(blockList, it, passManager);

                LOG_DEBUG(std::format("{:#^50} (result: {})", "Pass result", result), "MirPassManager");
                LOG_DEBUG(MirPrinter().printToString(block), "MirPassManager");

                changed |= result;

                if (pass->getPassType() == MirPassType::Transform)
                {
                    invalidateAllAnalyses();
                }
            }
        }

        // Cascade down into the instructions ONCE per block
        changed |= run(block->getInstructions(), block->getInstructions()->begin(), passManager);

        ++it;
    }

    return changed;
}

bool MirPassManager::run(TypedPoolLinkedList<struct MirInstruction> *instrList,
                         TypedPoolLinkedList<struct MirInstruction>::Iterator it,
                         struct MirPassManager *passManager)
{
    bool changed = false;

    // We must loop through all instructions in the list
    while (it != instrList->end())
    {
        // Run all Instruction-level passes on the current instruction
        for (auto &pass : m_passes)
        {
            if (pass->getIterationPlace() == MirPassIterationPlace::Instruction)
            {
                LOG_DEBUG(std::format("{:#^50} ({})", "Running instruction pass", pass->getName()), "MirPassManager");
                LOG_DEBUG(MirPrinter().printToString(*it), "MirPassManager");

                bool result = pass->run(instrList, it, passManager);

                LOG_DEBUG(std::format("{:#^50} (result: {})", "Pass result", result), "MirPassManager");
                LOG_DEBUG(MirPrinter().printToString(*it), "MirPassManager");

                changed |= result;

                if (pass->getPassType() == MirPassType::Transform)
                {
                    invalidateAllAnalyses();
                }
            }
        }

        ++it;
    }

    return changed;
}