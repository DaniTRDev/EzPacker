#ifndef EZTRIPLE_INSERTION_TRACKER_H
#define EZTRIPLE_INSERTION_TRACKER_H

#include "EzTripleCommon.h"
#include "Block/MirBlock.h"
#include "HelperClasses/IntrusiveLinkedList.h"
#include "Instruction/MirInstruction.h"

#include <vector>

/**
 * Tracks instruction insertions and removals around a target instruction during legalization transformations.
 * Allows newly produced instructions to be identified in O(k) time where k is the number of inserted instructions.
 */
class InsertionTracker
{
  public:
    InsertionTracker(MirBlock *block, MirInstruction *targetInst) :
        m_block(block),
        m_targetInst(targetInst),
        m_prevBefore(targetInst ? targetInst->getPrev() : nullptr),
        m_nextBefore(targetInst ? targetInst->getNext() : nullptr)
    {
    }

    IntrusiveLinkedList<MirInstruction>::iterator getIterator() const
    {
        if (!m_block || !m_targetInst)
            return {};
        return IntrusiveLinkedList<MirInstruction>::iterator(m_targetInst, &m_block->getInstructions());
    }

    MirInstruction *getTargetInstruction() const { return m_targetInst; }

    std::vector<MirInstruction *> getProducedInstructions() const
    {
        std::vector<MirInstruction *> produced;
        if (!m_block)
            return produced;

        // Walk from after m_prevBefore (or head of block) up to m_nextBefore
        MirInstruction *curr = nullptr;
        if (m_prevBefore)
        {
            curr = m_prevBefore->getNext();
        }
        else
        {
            auto it = m_block->getInstructions().begin();
            if (it != m_block->getInstructions().end())
            {
                curr = *it;
            }
        }

        while (curr && curr != m_nextBefore)
        {
            if (curr != m_targetInst && !curr->isErased())
            {
                produced.push_back(curr);
            }
            curr = curr->getNext();
        }

        return produced;
    }

  private:
    MirBlock *m_block{ nullptr };
    MirInstruction *m_targetInst{ nullptr };
    MirInstruction *m_prevBefore{ nullptr };
    MirInstruction *m_nextBefore{ nullptr };
};

#endif // EZTRIPLE_INSERTION_TRACKER_H
