#include "Operand/MirRegisterBank.h"
#include "Operand/MirRegisterClass.h"

/**
 * Initializes a new register bank with a symbolic bank name and memory allocator.
 */
MirRegisterBank::MirRegisterBank(const char *name, std::pmr::memory_resource *alloc) : m_name(name), m_classes(alloc) {}

/**
 * Adds a register class to this bank if it is not already present.
 */
bool MirRegisterBank::addClass(const std::string_view &name, MirRegisterClass *_class)
{
    auto it = m_classes.find(name);

    if (it != m_classes.end())
        return false;

    m_classes[name] = _class;
    return true;
}

/**
 * Returns the name of the register bank.
 */
const char *MirRegisterBank::getName() const { return m_name; }

/**
 * Retrieves a register class by its name, returning nullptr if not registered.
 */
MirRegisterClass *MirRegisterBank::getClass(const std::string_view &name) const
{
    auto it = m_classes.find(name);

    if (it == m_classes.end())
        return nullptr;

    return it->second;
}

/**
 * Returns the collection of all register classes contained within this bank.
 */
const std::pmr::unordered_map<std::string_view, MirRegisterClass *> &MirRegisterBank::getClasses() const
{
    return m_classes;
}