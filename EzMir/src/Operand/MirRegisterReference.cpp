#include "Operand/MirRegisterClass.h"
#include "Operand/MirRegisterReference.h"

MirRegisterRef::MirRegisterRef(size_t id, bool isVirtual, MirRegisterClass *_class) :
    m_virtual(isVirtual), m_class(_class), m_id(id)
{
}

MirRegisterRef::MirRegisterRef(MirRegisterClass *_class, size_t id) : m_virtual(false), m_class(_class), m_id(id) {}

bool MirRegisterRef::isVirtual() const { return m_virtual; }
bool MirRegisterRef::isPhysical() const { return !m_virtual; }

MirRegisterClass *MirRegisterRef::getClass() const { return m_class; }

MirRegisterRef MirRegisterRef::vreg(size_t id) { return MirRegisterRef(id); }
MirRegisterRef MirRegisterRef::vreg(size_t id, class MirRegisterClass *_class)
{
    return MirRegisterRef(id, true, _class);
}
MirRegisterRef MirRegisterRef::preg(class MirRegisterDescriptor *desc)
{
    return MirRegisterRef(desc->m_id, false, desc->m_owner);
}

size_t MirRegisterRef::getId() const { return m_id; }

void MirRegisterRef::setClass(MirRegisterClass *_class) { m_class = _class; }

bool MirRegisterRef::operator==(const MirRegisterRef &other) const
{
    if (m_virtual != other.m_virtual || m_id != other.m_id)
        return false;

    // Virtual registers don't have a class; physical registers must match class
    return m_virtual || (m_class == other.m_class);
}

bool MirRegisterRef::operator!=(const MirRegisterRef &other) const { return !(*this == other); }

bool MirRegisterRef::operator<(const MirRegisterRef &other) const
{
    if (m_virtual != other.m_virtual)
        return m_virtual < other.m_virtual;

    if (m_id != other.m_id)
        return m_id < other.m_id;

    // Compare class pointers for physical registers
    return !m_virtual && (m_class < other.m_class);
}
