#include "Block/MirBlockInstructionQuery.h"

MirBlockInstructionQuery::MirBlockInstructionQuery(const struct MirBlock *block) : m_block(block) {}

MirBlockInstructionQuery &MirBlockInstructionQuery::predicate(const InstructionQuery::Predicate &pred)
{
    m_predicates.push_back(pred);
    return *this;
}

MirInstruction *MirBlockInstructionQuery::findFirst() const
{
    const auto &instrList = m_block->getInstructions();
    for (auto &instr : instrList)
    {
        bool passed = true;
        for (const auto &pred : m_predicates)
        {
            if (!pred(instr))
            {
                passed = false;
                break;
            }
        }

        if (passed)
        {
            return instr;
        }
    }

    return nullptr;
}

void MirBlockInstructionQuery::forEach(const std::function<void(MirInstruction *)> &callback) const
{
    const auto &instrList = m_block->getInstructions();
    for (auto &instr : instrList)
    {
        bool passed = true;
        for (const auto &pred : m_predicates)
        {
            if (!pred(instr))
            {
                passed = false;
                break;
            }
        }

        if (passed)
        {
            callback(instr);
        }
    }
}

std::pmr::vector<MirInstruction *> MirBlockInstructionQuery::findAll() const
{
    std::pmr::vector<MirInstruction *> result(m_block->getInstructions().get_allocator());
    forEach([&result](MirInstruction *instr) { result.push_back(instr); });

    return result;
}
