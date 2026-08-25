#include "Instruction/MirTargetInstructionDesc.h"

/**
 * Initializes the target instruction descriptor with its assembly name, target ID, operand access flags, and implicit defs/uses.
 */
MirTargetInstructionDesc::MirTargetInstructionDesc(const char *name,
                                                   size_t id,
                                                   std::initializer_list<MirOperandFlag> operandFlags,
                                                   std::initializer_list<class MirRegisterRef> implicitDefs,
                                                   std::initializer_list<class MirRegisterRef> implicitUses) :
    m_name(name), m_id(id), m_implicitDefs(implicitDefs), m_implicitUses(implicitUses)
{
    m_operandsFlags.insert(m_operandsFlags.begin(), operandFlags.begin(), operandFlags.end());
}

/**
 * Returns the target assembly mnemonic name.
 */
const char *MirTargetInstructionDesc::getName() const { return m_name; }

/**
 * Returns the target opcode identifier.
 */
size_t MirTargetInstructionDesc::getId() const { return m_id; }

/**
 * Returns the vector of operand dataflow access flags.
 */
const std::vector<MirOperandFlag> &MirTargetInstructionDesc::getOperandsFlags() const { return m_operandsFlags; }

/**
 * Returns the vector of implicit hardware register definitions.
 */
const std::vector<class MirRegisterRef> &MirTargetInstructionDesc::getImplicitDefs() const { return m_implicitDefs; }

/**
 * Returns the vector of implicit hardware register uses.
 */
const std::vector<class MirRegisterRef> &MirTargetInstructionDesc::getImplicitUses() const { return m_implicitUses; }