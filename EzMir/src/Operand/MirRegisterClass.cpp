#include "Operand/MirRegisterClass.h"
#include "Operand/MirRegisterBank.h"

/**
 * Initializes a new register class with name, parent register bank, and memory allocator.
 */
MirRegisterClass::MirRegisterClass(const char *name, class MirRegisterBank *owner, std::pmr::memory_resource *alloc) :
    m_name(name), m_owner(owner), m_alloc(alloc), m_registers(alloc)
{
}

/**
 * Adds a new physical register descriptor with its bit width, offset, and sub-part aliases.
 * Returns true if inserted, false if a register with the specified name is already present.
 */
bool MirRegisterClass::addRegister(const std::string_view &name,
                                   size_t bitSize,
                                   size_t partOffsetInBits,
                                   std::initializer_list<MirRegisterDescriptor *> subParts)
{
    auto it = m_registers.find(name);
    if (it != m_registers.end())
        return false;

    std::pmr::polymorphic_allocator<MirRegisterDescriptor> alloc(m_alloc);

    auto desc =
            alloc.new_object<MirRegisterDescriptor>(name, this, bitSize, m_registers.size(), partOffsetInBits, m_alloc);
    desc->m_subParts.insert(desc->m_subParts.begin(), subParts.begin(), subParts.end());

    m_registers[name] = desc;
    return true;
}

/**
 * Retrieves the name of this register class.
 */
const char *MirRegisterClass::getName() const { return m_name; }

/**
 * Looks up a register descriptor by its mnemonic/name.
 */
MirRegisterDescriptor *MirRegisterClass::getReg(const std::string_view &name) const
{
    auto it = m_registers.find(name);
    if (it == m_registers.end())
        return nullptr;

    return it->second;
}

/**
 * Retrieves the table of register descriptors defined in this class.
 */
const std::pmr::unordered_map<std::string_view, MirRegisterDescriptor *> &MirRegisterClass::getRegs() const
{
    return m_registers;
}