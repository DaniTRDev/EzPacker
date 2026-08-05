#ifndef EZPACKER_MIRREGISTERREFERENCE_H
#define EZPACKER_MIRREGISTERREFERENCE_H

#include "EzMirCommon.h"
#include "Type/MirType.h"

enum class RegisterRefClass : uint8_t
{
    Invalid = 0,
    GPR,
    FPR,
    MAX_REF_TYPE // Used to iterate over this enum
};

/**
 * This class is used to encapsulate register indexing without affecting MirRegister in a bad way.
 * A register reference is what its name says: a reference to a register. This reference holds an ID which is
 * used to know if the referenced register is virtual or physical; and a class, which is used internally to distinguish
 * registers.
 */
class RegisterRef
{
  public:
    constexpr RegisterRef() = default;
    constexpr RegisterRef(RegisterRefClass refClass, size_t id, bool isVirtual) :
        m_virtual(isVirtual), m_class(refClass), m_id(id)
    {
    }

    static constexpr RegisterRef vreg(RegisterRefClass refClass, size_t id) { return RegisterRef(refClass, id, true); }
    static constexpr RegisterRef preg(RegisterRefClass refClass, size_t id) { return RegisterRef(refClass, id, false); }

    static constexpr RegisterRef fromType(MirType *type, size_t id, bool isVirtual)
    {
        RegisterRefClass regClass = RegisterRefClass::Invalid;
        if (type->getKind() == MirTypeKind::Integer || type->getKind() == MirTypeKind::Pointer)
        {
            regClass = RegisterRefClass::GPR;
        }
        else if (type->getKind() == MirTypeKind::FloatingPoint)
        {
            regClass = RegisterRefClass::FPR;
        }
        return RegisterRef(regClass, id, isVirtual);
    }

    constexpr bool isVirtual() const { return m_virtual; }
    constexpr bool isPhysical() const { return !m_virtual; }
    constexpr RegisterRefClass getClass() const { return m_class; }
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
    RegisterRefClass m_class{ RegisterRefClass::Invalid };
    size_t m_id{ MIRID_INVALID };
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
