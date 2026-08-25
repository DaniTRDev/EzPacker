#include "Block/MirBlock.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"

MirBlock::MirBlock(MirId id, SourceReference *sourceRef, MirFunction *owner, const std::pmr::string &name) :
    m_owner(owner), m_id(id), m_sourceRef(sourceRef), m_name(name)
{
}

IntrusiveLinkedList<MirInstruction> &MirBlock::getInstructions() { return m_instructions; }

const IntrusiveLinkedList<MirInstruction> &MirBlock::getInstructions() const { return m_instructions; }

IntrusiveLinkedList<MirInstruction> *MirBlock::getInstructionsPtr() { return &m_instructions; }

IntrusiveLinkedList<MirInstruction>::iterator MirBlock::begin() { return m_instructions.begin(); }

IntrusiveLinkedList<MirInstruction>::iterator MirBlock::end() { return m_instructions.end(); }

MirBlock *MirBlock::getPrev() const { return m_prev; }

MirBlock *MirBlock::getNext() const { return m_next; }

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

void MirBlock::setNext(MirBlock *next) { m_next = next; }

void MirBlock::setOwner(MirFunction *func) { m_owner = func; }

void MirBlock::setPrev(MirBlock *prev) { m_prev = prev; }

const std::pmr::string &MirBlock::getName() const { return m_name; }
