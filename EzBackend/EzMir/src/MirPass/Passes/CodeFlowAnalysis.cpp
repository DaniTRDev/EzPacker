#include "MirPass/Passes/CodeFlowAnalysis.h"

CodeFlowAnalysis::CodeFlowAnalysis(MirEmitter *emitter) : m_emitter(emitter) {}

bool CodeFlowAnalysis::run(MirFunction *func, MirPassManager *pm)
{
    m_result.m_successors.clear();
    m_result.m_predecessors.clear();

    TypedPoolSlice<MirBlock> *blocks = func->getBlocks();
    if (!blocks || blocks->m_numElems == 0)
        return false;
    
    for (auto it = blocks->begin(); it != blocks->end(); ++it)
    {
        MirBlock *currentBlock = *it;

        TypedPoolSlice<MirInstruction> *instructions = currentBlock->getInstructions();
        MirInstruction *terminator = nullptr;
        uint32_t terminatorFlags = 0;

        // Extract last instruction (terminator).
        if (instructions && instructions->m_numElems > 0)
        {
            terminator = *(instructions->rbegin());
            terminatorFlags = terminator->getFlags();
        }

        // Query the next block, just in case there's a fallthrough (conditional jump).
        auto nextIt = it;
        ++nextIt;
        MirBlock *nextBlock = (nextIt != blocks->end()) ? *nextIt : nullptr;

        if (terminator && (terminatorFlags & MirInstructionFlags::IsBranch))
        {
            if ((terminatorFlags & MirInstructionFlags::ReadsCPUFlags) == 0)
            {
                // Terminator is an UNCONDITIONAL BRANCH.
                MirBlock *target = getTargetJumpBlock(terminator);
                addEdge(currentBlock, target);
            }
            else
            {
                // Terminator is a CONDITIONAL BRANCH.
                MirBlock *target = getTargetJumpBlock(terminator);
                addEdge(currentBlock, target);

                // Fallthrough to the next block.
                if (nextBlock)
                    addEdge(currentBlock, nextBlock);
            }
        }
        else if (terminator && (terminatorFlags & MirInstructionFlags::IsReturn))
        {
            // No successors, function ends here.
        }
        else
        {
            // Fallthrough natural
            if (nextBlock)
            {
                addEdge(currentBlock, nextBlock);
            }
        }
    }

    return true;
}

const ControlFlowResult &CodeFlowAnalysis::getResult() const { return m_result; }

void CodeFlowAnalysis::addEdge(MirBlock *from, MirBlock *to) {}

MirBlock *CodeFlowAnalysis::getTargetJumpBlock(const MirInstruction *inst) const
{
    if (!inst->hasOperands())
    {
        return nullptr;
    }

    MirOperand *op = inst->getOperands()->get<MirOperand>(0);
    MirReference *ref = op->getReference();

    if (!ref)
        return nullptr;

    return m_emitter->getContext()->getBlockFromRef(*ref);
}
