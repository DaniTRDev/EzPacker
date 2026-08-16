#include "Block/MirBlock.h"

MirBlock::MirBlock(size_t id,
                   SourceReference *sourceRef,
                   std::pmr::list<MirInstruction *> instructions,
                   class MirFunction *owner,
                   const std::pmr::string &name) :
    m_owner(owner), m_id(id), m_sourceRef(sourceRef), m_instructions(std::move(instructions)), m_name(name)
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

class MirFunction *MirBlock::getOwner() const { return m_owner; }

size_t MirBlock::getId() const { return m_id; }

SourceReference *MirBlock::getSourceRef() const { return m_sourceRef; }

void MirBlock::setOwner(MirFunction *func) { m_owner = func; }

std::pmr::list<MirInstruction *> &MirBlock::getInstructions() { return m_instructions; }

const std::pmr::list<MirInstruction *> &MirBlock::getInstructions() const { return m_instructions; }

std::pmr::list<MirInstruction *> *MirBlock::getInstructionsPtr() { return &m_instructions; }

const std::pmr::string &MirBlock::getName() const { return m_name; }
