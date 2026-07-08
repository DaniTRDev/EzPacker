#include "Block/MirBlock.h"

MirBlock::MirBlock(size_t id,
                   SourceReference *sourceRef,
                   std::pmr::list<MirInstruction *> instructions,
                   const std::pmr::string &name) :
    m_id(id), m_sourceRef(sourceRef), m_instructions(std::move(instructions)), m_name(name)
{
}

MirInstruction *MirBlock::at(size_t index)
{
    if (index >= m_instructions.size())
        return nullptr;

    auto it = m_instructions.begin();
    std::advance(it, index);

    return *it;
}

size_t MirBlock::getId() const { return m_id; }

SourceReference *MirBlock::getSourceRef() const { return m_sourceRef; }

std::pmr::list<MirInstruction *> &MirBlock::getInstructions() { return m_instructions; }

const std::pmr::list<MirInstruction *> &MirBlock::getInstructions() const { return m_instructions; }

std::pmr::list<MirInstruction *> *MirBlock::getInstructionsPtr() { return &m_instructions; }

const std::pmr::string &MirBlock::getName() const { return m_name; }
