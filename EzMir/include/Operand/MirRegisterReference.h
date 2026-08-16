#ifndef EZMIR_MIR_REGISTER_REFERENCE_H
#define EZMIR_MIR_REGISTER_REFERENCE_H

#include "EzMirCommon.h"

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
class MirRegisterRef
{
  public:
    constexpr MirRegisterRef() = default;
    MirRegisterRef(size_t id, bool isVirtual = true, class MirRegisterClass *_class = nullptr);

    /**
     * Constructor for PHYSICAL registers.
     */
    MirRegisterRef(class MirRegisterClass *_class, size_t id);

    bool isVirtual() const;
    bool isPhysical() const;

    class MirRegisterClass *getClass() const;

    /**
     * Builds a FULL virtual register ref with the given ID.
     */
    static MirRegisterRef vreg(size_t id);

    /**
     * Builds a virtual register ref with a physical class assigned.
     */
    static MirRegisterRef vreg(size_t id, class MirRegisterClass *_class);

    /**
     * Builds a physical register ref out of the given register descriptor.
     */
    static MirRegisterRef preg(class MirRegisterDescriptor *desc);

    size_t getId() const;

    void setClass(class MirRegisterClass *_class);

    bool operator==(const MirRegisterRef &other) const;

    bool operator!=(const MirRegisterRef &other) const;

    bool operator<(const MirRegisterRef &other) const;

  private:
    bool m_virtual{ true };
    class MirRegisterClass *m_class{ nullptr };
    size_t m_id{ MIRID_INVALID };
};

// Standard hash implementation for unordered containers
namespace std
{
template <> struct hash<MirRegisterRef>
{
    size_t operator()(const MirRegisterRef &reg) const noexcept
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

#endif // EZMIR_MIR_REGISTER_REFERENCE_H