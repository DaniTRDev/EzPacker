#ifndef EZPACKER_MIRREGISTERREFERENCE_H
#define EZPACKER_MIRREGISTERREFERENCE_H

#include "EzMirCommon.h"

/**
 * This class is used to encapsulate register indexing without affecting MirRegister in a bad way.
 * A register reference is what its name says: a reference to a register. This reference holds an ID which is
 * used to know if the referenced register is virtual or physical.
 */
class RegisterRef
{
  public:
    constexpr RegisterRef() = default;
    constexpr RegisterRef(size_t id, bool isVirtual) : m_virtual(isVirtual), m_id(id) {}

    static constexpr RegisterRef vreg(size_t id) { return RegisterRef(id, true); }
    static constexpr RegisterRef preg(size_t id) { return RegisterRef(id, false); }

    constexpr bool isVirtual() const { return m_virtual; }
    constexpr bool isPhysical() const { return !m_virtual; }
    constexpr size_t getId() const { return m_id; }

    constexpr bool operator==(const RegisterRef &other) const
    {
        return m_id == other.m_id && m_virtual == other.m_virtual;
    }
    constexpr bool operator!=(const RegisterRef &other) const { return !(*this == other); }
    constexpr bool operator<(const RegisterRef &other) const
    {
        if (m_virtual != other.m_virtual)
            return m_virtual < other.m_virtual;

        return m_id < other.m_id;
    }

  private:
    bool m_virtual{ true };
    size_t m_id{ 0 };
};

// Standard hash implementation so RegisterRef can key ordered containers.
namespace std
{
template <> struct hash<RegisterRef>
{
    size_t operator()(const RegisterRef &reg) const noexcept
    {
        uint64_t combined = (static_cast<uint64_t>(reg.getId()) << 1) | (reg.isVirtual() ? 1 : 0);
        return std::hash<uint64_t>{}(combined);
    }
};
} // namespace std

#endif // EZPACKER_MIRREGISTERREFERENCE_H
