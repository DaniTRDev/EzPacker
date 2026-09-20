#include "Block/MirBlock.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"

/**
 * Initializes a new basic block with its unique ID, source reference, owning function, and name.
 */
MirBlock::MirBlock(MirId id,
                   SourceReference *sourceRef,
                   MirFunction *owner,
                   std::pmr::memory_resource *alloc,
                   const std::pmr::string &name) :
    m_owner(owner), m_id(id), m_sourceRef(sourceRef), m_name(name), m_predecessors(alloc)
{
}

/**
 * Returns const reference to the block's intrusive instruction list.
 */
const IntrusiveLinkedList<MirInstruction> &MirBlock::getInstructions() const { return m_instructions; }

/**
 * Returns const iterator to the first instruction.
 */
IntrusiveLinkedList<MirInstruction>::const_iterator MirBlock::begin() const { return m_instructions.begin(); }

/**
 * Returns const iterator past the last instruction.
 */
IntrusiveLinkedList<MirInstruction>::const_iterator MirBlock::end() const { return m_instructions.end(); }

/**
 * Returns preceding basic block in the layout sequence.
 */
MirBlock *MirBlock::getPrev() const { return m_prev; }

/**
 * Returns subsequent basic block in the layout sequence.
 */
MirBlock *MirBlock::getNext() const { return m_next; }

/**
 * Retrieves the instruction at index by advancing through the intrusive list.
 */
MirInstruction *MirBlock::at(size_t index)
{
    if (index >= m_instructions.size())
        return nullptr;

    auto it = m_instructions.begin();
    std::advance(it, index);

    return *it;
}

/**
 * Returns parent function owning this basic block.
 */
MirFunction *MirBlock::getOwner() const { return m_owner; }

/**
 * Returns the unique MIR identifier of the block.
 */
MirId MirBlock::getId() const { return m_id; }

/**
 * Returns the source code reference.
 */
SourceReference *MirBlock::getSourceRef() const { return m_sourceRef; }

/**
 * Returns the total instruction count in this block.
 */
size_t MirBlock::getInstrCount() const { return m_instructions.size(); }

/**
 * Returns the label name of the block.
 */
const std::pmr::string &MirBlock::getName() const { return m_name; }
void MirBlock::setName(const std::pmr::string &name) { m_name = name; }

/**
 * Links the subsequent block in the intrusive sequence.
 */
void MirBlock::setNext(MirBlock *next) { m_next = next; }

/**
 * Sets the owning function pointer.
 */
void MirBlock::setOwner(MirFunction *func) { m_owner = func; }

/**
 * Links the preceding block in the intrusive sequence.
 */
void MirBlock::setPrev(MirBlock *prev) { m_prev = prev; }
