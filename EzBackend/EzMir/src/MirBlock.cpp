#include "MirBlock.h"

MirBlock::MirBlock(size_t id, TypedPoolLinkedList<MirInstruction> *instructions) : m_id(id), m_instructions(instructions) {}

TypedPoolLinkedList<MirInstruction> *MirBlock::getInstructions() const { return m_instructions; }

size_t MirBlock::getId() const { return m_id; }
