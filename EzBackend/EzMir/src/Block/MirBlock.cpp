#include "Block/MirBlock.h"

MirBlock::MirBlock(size_t id, SourceReference *sourceRef, std::pmr::list<MirInstruction *> instructions) :
    m_id(id), m_sourceRef(sourceRef), m_instructions(std::move(instructions))
{
}

size_t MirBlock::getId() const { return m_id; }

SourceReference *MirBlock::getSourceRef() const { return m_sourceRef; }

std::pmr::list<MirInstruction *> &MirBlock::getInstructions() { return m_instructions; }

const std::pmr::list<MirInstruction *> &MirBlock::getInstructions() const { return m_instructions; }
