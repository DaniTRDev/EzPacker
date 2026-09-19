#ifndef EZMIR_MIR_REGISTER_CLASS_H
#define EZMIR_MIR_REGISTER_CLASS_H

#include "EzMirCommon.h"

/**
 * Descriptor storing physical register metadata, size, parent class, and sub-register aliasing relations.
 *
 * The m_partOffsetInBits member indicates bit offset when this register is a sub-slice of a larger register.
 * For instance:
 *  - AL (m_partOffsetInBits: 0, m_bitSize: 8, m_subParts: empty)
 *  - AH (m_partOffsetInBits: 8, m_bitSize: 8, m_subParts: empty)
 *  - AX (m_partOffsetInBits: 0, m_bitSize: 16, m_subParts: [AL, AH])
 *  - EAX (m_partOffsetInBits: 0, m_bitSize: 32, m_subParts: [AX])
 */
struct MirRegisterDescriptor
{
    /**
     * Diagnostic and assembly print name of the hardware register.
     */
    std::string_view m_name;

    /**
     * Owning register class.
     */
    class MirRegisterClass *m_owner;

    /**
     * Bit width of the physical register.
     */
    size_t m_bitSize;

    /**
     * Unique index within the owning register class.
     */
    size_t m_id;

    /**
     * Bit offset of this sub-register inside its enclosing parent register.
     */
    size_t m_partOffsetInBits;

    /**
     * Hardware encoding of this register. This is the value that the encoder places in
     * ModR/M reg/rm fields, SIB fields, or the opcode+rd low bits. It is target-defined:
     * for x86-64 GPRs it matches 0..15, for XMM0..15 it matches 0..15, and so on.
     *
     * When not explicitly provided by the target's register definition, it defaults to m_id.
     */
    uint32_t m_hwEncoding;

    /**
     * Sub-register slices that compose this register.
     */
    std::pmr::vector<MirRegisterDescriptor *> m_subParts;

    /**
     * Sentinel used to request that m_hwEncoding default to the descriptor's m_id.
     */
    static constexpr uint32_t INVALID_HW_ENCODING = UINT32_MAX;

    /**
     * Constructs a register descriptor.
     */
    MirRegisterDescriptor(const std::string_view &name,
                          class MirRegisterClass *owner,
                          size_t bitSize,
                          size_t id,
                          size_t partOffsetInBits,
                          std::pmr::memory_resource *alloc,
                          uint32_t hwEncoding = INVALID_HW_ENCODING) :
        m_name(name), m_owner(owner), m_bitSize(bitSize), m_id(id), m_partOffsetInBits(partOffsetInBits),
        m_hwEncoding(hwEncoding == INVALID_HW_ENCODING ? static_cast<uint32_t>(id) : hwEncoding), m_subParts(alloc)
    {
    }

    /**
     * Appends a sub-register slice to this descriptor. Used by generated register info to
     * reconstruct aliasing hierarchies (e.g. rax -> eax -> ax -> al).
     */
    void addSubPart(MirRegisterDescriptor *child)
    {
        if (child)
        {
            m_subParts.push_back(child);
        }
    }
};

/**
 * Abstraction for register classes within a hardware register bank (e.g. GPR8, GPR16, GPR32, GPR64, FPR32, FPR64).
 *
 * Register IDs are unique within each bank but shared across classes in the same bank.
 */
class MirRegisterClass
{
  public:
    /**
     * Constructs an empty register class with the given name, parent bank, and memory resource.
     */
    MirRegisterClass(const char *name, class MirRegisterBank *owner, std::pmr::memory_resource *alloc);

    /**
     * Returns the name of the register class.
     */
    const char *getName() const;

    /**
     * Inserts a register descriptor into this class if not already registered.
     * Returns true upon successful insertion, false if a register with the same name exists.
     *
     * @param hwEncoding Explicit hardware encoding for the encoder; defaults to m_id to
     *                   preserve the behavior of targets that use identity-mapped encodings.
     */
    bool addRegister(const std::string_view &name,
                     size_t bitSize,
                     size_t partOffsetInBits,
                     std::initializer_list<MirRegisterDescriptor *> subParts,
                     uint32_t hwEncoding = MirRegisterDescriptor::INVALID_HW_ENCODING);

    /**
     * Looks up a register descriptor by name within this class; returns nullptr if not found.
     */
    MirRegisterDescriptor *getReg(const std::string_view &name) const;

    /**
     * Returns the map of register names to their descriptors.
     */
    const std::pmr::unordered_map<std::string_view, MirRegisterDescriptor *> &getRegs() const;

  private:
    /**
     * Name identifier for this register class (e.g., "GPR32").
     */
    const char *m_name;

    /**
     * Owning hardware register bank (e.g., GPR, FPR).
     */
    class MirRegisterBank *m_owner;

    /**
     * Memory resource used for descriptor allocations.
     */
    std::pmr::memory_resource *m_alloc;

    /**
     * Lookup table mapping register names to descriptors.
     */
    std::pmr::unordered_map<std::string_view, MirRegisterDescriptor *> m_registers;
};

#endif // EZMIR_MIR_REGISTER_CLASS_H