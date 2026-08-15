#ifndef EZPACKER_MIRREGISTERREFERENCE_H
#define EZPACKER_MIRREGISTERREFERENCE_H

#include "EzMirCommon.h"
#include "Type/MirType.h"
#include "MirRegisterClass.h"
#include <functional>

/**
 * This class is used to encapsulate register references.
 * Virtual registers do not contain a register class, as this is only assigned to PHYSICAL.
 * Physical registers have a register class, which also links them to a HW-defined register bank.
 *
 * Important note, IDs are unique within each bank but shared across classes. This means that register ID 1 from bank 1
 * is different to register ID 1 from bank 2; AND register ID 1, with class 1 is different of register ID 1 with
 * class 2.
 *
 *
 * When a VIRTUAL register ref has a class, this means the register reference passed ISel phase.
 */
class RegisterRef
{
  public:
    constexpr RegisterRef() = default;

    constexpr RegisterRef(size_t id, bool isVirtual = true, MirRegisterClass *_class = nullptr) :
        m_virtual(isVirtual), m_class(_class), m_id(id)
    {
    }

    /**
     * Constructor for PHYSICAL registers.
     */
    constexpr RegisterRef(MirRegisterClass *_class, size_t id) : m_virtual(false), m_class(_class), m_id(id) {}

    static constexpr RegisterRef vreg(size_t id) { return RegisterRef(id); }
    static constexpr RegisterRef vreg(size_t id, MirRegisterClass *_class) { return RegisterRef(id, true, _class); }

    static constexpr RegisterRef preg(MirRegisterDescriptor *desc)
    {
        if (desc->m_owner == nullptr)
            throw std::runtime_error("LOL");

        return RegisterRef(desc->m_id, false, desc->m_owner);
    }

    constexpr bool isVirtual() const { return m_virtual; }
    constexpr bool isPhysical() const { return !m_virtual; }

    constexpr MirRegisterClass *getClass() const { return m_class; }
    constexpr size_t getId() const { return m_id; }

    void setClass(MirRegisterClass *_class) { m_class = _class; }

    constexpr bool operator==(const RegisterRef &other) const
    {
        if (m_virtual != other.m_virtual || m_id != other.m_id)
            return false;

        // Virtual registers don't have a class; physical registers must match class
        return m_virtual || (m_class == other.m_class);
    }

    constexpr bool operator!=(const RegisterRef &other) const { return !(*this == other); }

    constexpr bool operator<(const RegisterRef &other) const
    {
        if (m_virtual != other.m_virtual)
            return m_virtual < other.m_virtual;

        if (m_id != other.m_id)
            return m_id < other.m_id;

        // Compare class pointers for physical registers
        return !m_virtual && (m_class < other.m_class);
    }

  private:
    bool m_virtual{ true };
    MirRegisterClass *m_class{ nullptr };
    size_t m_id{ MIRID_INVALID };
};

// Standard hash implementation for unordered containers
namespace std
{
template <> struct hash<RegisterRef>
{
    size_t operator()(const RegisterRef &reg) const noexcept
    {
        // Hash the ID and virtual status
        size_t seed = std::hash<size_t>{}(reg.getId());
        seed ^= std::hash<bool>{}(reg.isVirtual()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

        // Hash the class pointer for physical registers
        if (reg.isPhysical())
        {
            seed ^= std::hash<const MirRegisterClass *>{}(reg.getClass()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        return seed;
    }
};
} // namespace std

#endif // EZPACKER_MIRREGISTERREFERENCE_H