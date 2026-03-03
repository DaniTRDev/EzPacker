#include "Instruction/MirInstruction.h"

MirInstruction::MirInstruction(MirInstructionOpCode opcode, TypedPoolSlice<MirOperand> *operands) :
    m_opcode(opcode), m_operands(operands)
{
}

bool MirInstruction::hasOperands() const { return m_operands && m_operands->m_numElems > 0; }

const MirInstructionMetadata &MirInstruction::getMetadata() const { return getMeta(getOpCode()); }

MirInstructionOpCode MirInstruction::getOpCode() const { return m_opcode; }

TypedPoolSlice<MirOperand> *MirInstruction::getOperands() { return m_operands; }

uint32_t MirInstruction::getFlags() const { return getMeta(getOpCode()).m_flags; }
