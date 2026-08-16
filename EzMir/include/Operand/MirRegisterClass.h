#ifndef EZMIR_MIR_REGISTER_CLASS_H
#define EZMIR_MIR_REGISTER_CLASS_H

#include "EzMirCommon.h"

/**
 * This descriptor contains information about a register.
 *
 * m_partOffsetInBits indicates whenever this register is the lower/upper part of another register in REVERSE ORDER.
 * Here's an example:
 *  - Imagine AL(m_partOffsetInBits:0) and AH(m_partOffsetInBits:8) (m_subParts = empty)
 *  - Now imagine EAX (m_partOffsetInBits:0) (m_subParts = AL, AH).
 */
struct MirRegisterDescriptor
{
    std::string_view m_name;
    class MirRegisterClass *m_owner;
    size_t m_bitSize;
    size_t m_id;
    size_t m_partOffsetInBits;
    std::pmr::vector<MirRegisterDescriptor *> m_subParts;

    MirRegisterDescriptor(const std::string_view &name,
                          class MirRegisterClass *owner,
                          size_t bitSize,
                          size_t id,
                          size_t partOffsetInBits,
                          std::pmr::memory_resource *alloc) :
        m_name(name), m_owner(owner), m_bitSize(bitSize), m_id(id), m_partOffsetInBits(partOffsetInBits),
        m_subParts(alloc)
    {
    }
};

/**
 *  This structure abstract register classes: GPR8, GPR16, ..., FPR8, Special classes (imagine a coprocessor having a
 * special set of registers...)
 *
 * The bank is where the HW store 1 or more classes of registers: register bank(GPR). Classes: GPR8, GPR16, GPR32,
 * GPR64...
 *
 * Important note, IDs are unique within each bank but shared across classes. This means that register ID 1 from bank
 * 1 is different to register ID 1 from bank 2; AND register ID 1, with class 1 is different of register ID 1 with
 * class 2.
 */
class MirRegisterClass
{
  public:
    /**
     * Creates an empty register class with the given name, owning bank, and allocator.
     */
    MirRegisterClass(const char *name, class MirRegisterBank *owner, std::pmr::memory_resource *alloc);

    /**
     * Returns the name of the class.
     */
    const char *getName() const;

    /**
     * Adds a register, if not added already. If the register is already present in this class, false is returned.
     * This function returns true if the register was correctly inserted.
     */
    bool addRegister(const std::string_view &name,
                     size_t bitSize,
                     size_t partOffsetInBits,
                     std::initializer_list<MirRegisterDescriptor *> subParts);

    /**
     * Returns the register with the given name. If no register matches, nullptr is returned.
     */
    MirRegisterDescriptor *getReg(const std::string_view &name) const;

    /**
     * Returns the register map.
     */
    const std::pmr::unordered_map<std::string_view, MirRegisterDescriptor *> &getRegs() const;

  private:
    const char *m_name;
    class MirRegisterBank *m_owner;
    std::pmr::memory_resource *m_alloc;
    std::pmr::unordered_map<std::string_view, MirRegisterDescriptor *> m_registers;
};

#endif // EZMIR_MIR_REGISTER_CLASS_H