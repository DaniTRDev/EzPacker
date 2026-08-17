#include "Block/MirBlock.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"

MirBlock::MirBlock(MirId id,
                   SourceReference *sourceRef,
                   std::pmr::list<class MirInstruction *> instructions,
                   MirFunction *owner,
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

MirFunction *MirBlock::getOwner() const { return m_owner; }

MirId MirBlock::getId() const { return m_id; }

SourceReference *MirBlock::getSourceRef() const { return m_sourceRef; }

size_t MirBlock::getInstrCount() const { return m_instructions.size(); }

void MirBlock::setOwner(MirFunction *func) { m_owner = func; }

std::pmr::list<MirInstruction *> &MirBlock::getInstructions() { return m_instructions; }

const std::pmr::list<MirInstruction *> &MirBlock::getInstructions() const { return m_instructions; }

std::pmr::list<MirInstruction *> *MirBlock::getInstructionsPtr() { return &m_instructions; }

std::pmr::list<MirInstruction *>::iterator MirBlock::begin() { return m_instructions.begin(); }

std::pmr::list<MirInstruction *>::iterator MirBlock::end() { return m_instructions.end(); }

const std::pmr::string &MirBlock::getName() const { return m_name; }
