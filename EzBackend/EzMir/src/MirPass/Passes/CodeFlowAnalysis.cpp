#include "MirPass/Passes/CodeFlowAnalysis.h"

CodeFlowAnalysis::CodeFlowAnalysis(MirEmitter *emitter) : m_emitter(emitter) {}

bool CodeFlowAnalysis::run(TypedPoolLinkedList<class MirBlock> *blockList,
                           TypedPoolLinkedList<class MirBlock>::Iterator it,
                           class MirPassManager *passManager)
{
    m_result.m_successors.clear();
    m_result.m_predecessors.clear();

    for (auto instrIt = blockList->begin(); instrIt != blockList->end(); ++instrIt)
    {
        MirBlock *currentBlock = *instrIt;

        TypedPoolLinkedList<MirInstruction> *instructions = currentBlock->getInstructions();
        MirInstruction *terminator = nullptr;
        MirInstructionFlags terminatorFlags = MirInstructionFlags::None;

        // Extract last instruction (terminator).
        if (instructions && instructions->m_numElems > 0)
        {
            terminator = *(instructions->rbegin());
            terminatorFlags = terminator->getFlags();
        }

        // Query the next block, just in case there's a fallthrough (conditional jump).
        auto nextIt = instrIt;
        ++nextIt;
        MirBlock *nextBlock = (nextIt != blockList->end()) ? *nextIt : nullptr;

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
    MirReference *ref = op->get<MirReference>();

    if (!ref)
        return nullptr;

    return m_emitter->getContext()->getBlockFromRef(*ref);
}

MirPassIterationPlace CodeFlowAnalysis::getIterationPlace() const { return MirPassIterationPlace::Block; }
