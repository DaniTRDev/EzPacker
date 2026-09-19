#include "Instruction/MirTargetInstructionDesc.h"

/**
 * Initializes the target instruction descriptor with its assembly name, target ID, operand flags, operand classes,
 * implicit defs/uses, and target flags.
 */
MirTargetInstructionDesc::MirTargetInstructionDesc(const char *name,
                                                   size_t id,
                                                   std::initializer_list<MirOperandFlag> operandFlags,
                                                   std::initializer_list<MirRegisterClass *> operandClasses,
                                                   std::initializer_list<MirRegisterRef> implicitDefs,
                                                   std::initializer_list<MirRegisterRef> implicitUses,
                                                   MirInstructionFlags targetFlags) :
    m_name(name), m_id(id), m_operandClasses(operandClasses), m_implicitDefs(implicitDefs),
    m_implicitUses(implicitUses), m_targetFlags(targetFlags)
{
    m_operandsFlags.insert(m_operandsFlags.begin(), operandFlags.begin(), operandFlags.end());
}

/**
 * Backward-compatible constructor without operand classes.
 */
MirTargetInstructionDesc::MirTargetInstructionDesc(const char *name,
                                                   size_t id,
                                                   std::initializer_list<MirOperandFlag> operandFlags,
                                                   std::initializer_list<MirRegisterRef> implicitDefs,
                                                   std::initializer_list<MirRegisterRef> implicitUses,
                                                   MirInstructionFlags targetFlags) :
    m_name(name), m_id(id), m_implicitDefs(implicitDefs), m_implicitUses(implicitUses),
    m_targetFlags(targetFlags)
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
 * Returns the vector of operand register class constraints.
 */
const std::vector<MirRegisterClass *> &MirTargetInstructionDesc::getOperandClasses() const { return m_operandClasses; }

/**
 * Returns the register class constraint for the operand at index, or nullptr.
 */
MirRegisterClass *MirTargetInstructionDesc::getOperandClass(size_t index) const
{
    if (index < m_operandClasses.size())
    {
        return m_operandClasses[index];
    }
    return nullptr;
}

void MirTargetInstructionDesc::setOperandClass(size_t index, MirRegisterClass *regClass)
{
    if (index >= m_operandClasses.size())
    {
        m_operandClasses.resize(index + 1, nullptr);
    }
    m_operandClasses[index] = regClass;
}

void MirTargetInstructionDesc::setOperandClasses(std::vector<MirRegisterClass *> classes)
{
    m_operandClasses = std::move(classes);
}

/**
 * Returns the vector of implicit hardware register definitions.
 */
const std::vector<class MirRegisterRef> &MirTargetInstructionDesc::getImplicitDefs() const { return m_implicitDefs; }

/**
 * Returns the vector of implicit hardware register uses.
 */
const std::vector<class MirRegisterRef> &MirTargetInstructionDesc::getImplicitUses() const { return m_implicitUses; }

/**
 * Returns the target instruction flags.
 */
MirInstructionFlags MirTargetInstructionDesc::getTargetFlags() const { return m_targetFlags; }

/**
 * Returns the index into the generated encoding table, or INVALID_ENCODING_ID.
 */
size_t MirTargetInstructionDesc::getEncodingId() const { return m_encodingId; }

/**
 * Associates this descriptor with a generated encoding table entry.
 */
void MirTargetInstructionDesc::setEncodingId(size_t id) { m_encodingId = id; }