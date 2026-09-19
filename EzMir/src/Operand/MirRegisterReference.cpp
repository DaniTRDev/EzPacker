#include "Operand/MirRegisterClass.h"
#include "Operand/MirRegisterReference.h"

/**
 * Initializes a register reference with ID, virtuality flag, and register class constraint.
 */
MirRegisterRef::MirRegisterRef(size_t id, bool isVirtual, MirRegisterClass *_class) :
    m_virtual(isVirtual), m_class(_class), m_id(id)
{
}

/**
 * Initializes a physical register reference bound to a specific register class and physical ID.
 */
MirRegisterRef::MirRegisterRef(MirRegisterClass *_class, size_t id) : m_virtual(false), m_class(_class), m_id(id) {}

/**
 * Returns true if this is an unallocated virtual register.
 */
bool MirRegisterRef::isVirtual() const { return m_virtual; }

/**
 * Returns true if this is a physical hardware register.
 */
bool MirRegisterRef::isPhysical() const { return !m_virtual; }

/**
 * Returns the register class constraint descriptor.
 */
MirRegisterClass *MirRegisterRef::getClass() const { return m_class; }

/**
 * Creates an unconstrained virtual register reference with the given ID.
 */
MirRegisterRef MirRegisterRef::vreg(size_t id) { return MirRegisterRef(id); }

/**
 * Creates a virtual register reference with an explicit target register class assigned.
 */
MirRegisterRef MirRegisterRef::vreg(size_t id, class MirRegisterClass *_class)
{
    return MirRegisterRef(id, true, _class);
}

/**
 * Creates a physical register reference from a hardware register descriptor.
 */
MirRegisterRef MirRegisterRef::preg(class MirRegisterDescriptor *desc)
{
    return MirRegisterRef(desc->m_id, false, desc->m_owner);
}

/**
 * Retrieves the numeric identifier of the register.
 */
size_t MirRegisterRef::getId() const { return m_id; }

/**
 * Assigns or updates the register class descriptor.
 */
void MirRegisterRef::setClass(MirRegisterClass *_class) { m_class = _class; }

/**
 * Checks equality between two register references. Virtual registers match by ID; physical registers must also match
 * class.
 */
bool MirRegisterRef::operator==(const MirRegisterRef &other) const
{
    if (m_virtual != other.m_virtual || m_id != other.m_id)
        return false;

    // Virtual registers don't have a class; physical registers must match class
    return m_virtual || (m_class == other.m_class);
}

/**
 * Inequality comparison operator.
 */
bool MirRegisterRef::operator!=(const MirRegisterRef &other) const { return !(*this == other); }

/**
 * Strict weak ordering comparison operator for sorting and associative container keys.
 */
bool MirRegisterRef::operator<(const MirRegisterRef &other) const
{
    if (m_virtual != other.m_virtual)
        return m_virtual < other.m_virtual;

    if (m_id != other.m_id)
        return m_id < other.m_id;

    // Compare class pointers for physical registers
    return !m_virtual && (m_class < other.m_class);
}
