#ifndef EZMIR_MIR_REGISTER_REFERENCE_H
#define EZMIR_MIR_REGISTER_REFERENCE_H

#include "EzMirCommon.h"
#include "Operand/MirRegisterClass.h"
#include <string_view>

/**
 * Encapsulates a reference to either a virtual register (SSA/pre-allocation) or a physical register
 * (hardware-assigned).
 *
 * Virtual registers originate without a register class. When instruction selection assigns a target register class,
 * the virtual register receives the associated MirRegisterClass descriptor.
 * Physical registers are always bound to a specific MirRegisterClass and bank.
 */
class MirRegisterRef
{
  public:
    /**
     * Default constructor creating an unassigned virtual register reference.
     */
    constexpr MirRegisterRef() = default;

    /**
     * Constructs a register reference with the specified identifier, virtual/physical flag, and optional class.
     */
    MirRegisterRef(size_t id, bool isVirtual = true, class MirRegisterClass *_class = nullptr);

    /**
     * Constructs a physical register reference with a bound register class and physical ID.
     */
    MirRegisterRef(class MirRegisterClass *_class, size_t id);

    /**
     * Returns true if this register reference is virtual (not yet allocated to hardware).
     */
    bool isVirtual() const;

    /**
     * Returns true if this register reference represents a physical hardware register.
     */
    bool isPhysical() const;

    /**
     * Returns the register class descriptor (assigned during/after instruction selection for virtuals, or inherent for
     * physicals).
     */
    class MirRegisterClass *getClass() const;

    /**
     * Factory function creating a purely virtual register reference with no initial class.
     */
    static MirRegisterRef vreg(size_t id);

    /**
     * Factory function creating a virtual register reference constrained to a specific physical register class.
     */
    static MirRegisterRef vreg(size_t id, class MirRegisterClass *_class);

    /**
     * Factory function creating a physical register reference from a hardware register descriptor.
     */
    static MirRegisterRef preg(class MirRegisterDescriptor *desc);

    /**
     * Returns the numeric identifier of the register.
     */
    size_t getId() const;

    /**
     * Sets or updates the register class constraint for this register reference.
     */
    void setClass(class MirRegisterClass *_class);

    /**
     * Equality operator comparing virtual flag, ID, and class pointer (for physical registers).
     */
    bool operator==(const MirRegisterRef &other) const;

    /**
     * Inequality operator.
     */
    bool operator!=(const MirRegisterRef &other) const;

    /**
     * Ordering operator enabling use as keys in ordered containers.
     */
    bool operator<(const MirRegisterRef &other) const;

  private:
    /**
     * Flag indicating if the register is virtual (true) or physical (false).
     */
    bool m_virtual{ true };

    /**
     * Associated hardware register class descriptor.
     */
    class MirRegisterClass *m_class{ nullptr };

    /**
     * Unique numeric register identifier.
     */
    size_t m_id{ MIRID_INVALID };
};

/**
 * Standard hash specialization for MirRegisterRef to enable hashing in std::unordered_map / std::unordered_set.
 */
namespace std
{
template <> struct hash<MirRegisterRef>
{
    /**
     * Computes a combined hash from the register ID, virtuality flag, and — for physical registers —
     * the register-class name. The class name (rather than its address) is hashed so unordered
     * container iteration, and therefore register allocation, is reproducible across runs;
     * `MirRegisterRef::operator<` orders by the same name for the same reason.
     */
    size_t operator()(const MirRegisterRef &reg) const noexcept
    {
        // Hash the ID and virtual status
        size_t seed = std::hash<size_t>{}(reg.getId());
        seed ^= std::hash<bool>{}(reg.isVirtual()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

        // Hash the physical register's class name, never its heap address.
        if (reg.isPhysical())
        {
            const MirRegisterClass *regClass = reg.getClass();
            const size_t classHash =
                regClass != nullptr ? std::hash<std::string_view>{}(std::string_view(regClass->getName())) : 0;
            seed ^= classHash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        return seed;
    }
};
} // namespace std

#endif // EZMIR_MIR_REGISTER_REFERENCE_H