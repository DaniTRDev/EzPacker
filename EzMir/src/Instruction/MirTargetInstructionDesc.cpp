#include "Instruction/MirTargetInstructionDesc.h"

MirTargetInstructionDesc::MirTargetInstructionDesc(const char *name,
                                                   size_t id,
                                                   std::initializer_list<MirOperandFlag> operandFlags,
                                                   std::initializer_list<class MirRegisterRef> implicitDefs,
                                                   std::initializer_list<class MirRegisterRef> implicitUses) :
    m_name(name), m_id(id), m_implicitDefs(implicitDefs), m_implicitUses(implicitUses)
{
    m_operandsFlags.insert(m_operandsFlags.begin(), operandFlags.begin(), operandFlags.end());
}

const char *MirTargetInstructionDesc::getName() const { return m_name; }

size_t MirTargetInstructionDesc::getId() const { return m_id; }

const std::vector<MirOperandFlag> &MirTargetInstructionDesc::getOperandsFlags() const { return m_operandsFlags; }

const std::vector<class MirRegisterRef> &MirTargetInstructionDesc::getImplicitDefs() const { return m_implicitDefs; }

const std::vector<class MirRegisterRef> &MirTargetInstructionDesc::getImplicitUses() const { return m_implicitUses; }