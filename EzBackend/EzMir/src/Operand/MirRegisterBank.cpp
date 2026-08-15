#include "Operand/MirRegisterBank.h"

MirRegisterBank::MirRegisterBank(const char *name, std::pmr::memory_resource *alloc) : m_name(name), m_classes(alloc) {}

bool MirRegisterBank::addClass(const std::string_view &name, MirRegisterClass *_class)
{
    auto it = m_classes.find(name);

    if (it != m_classes.end())
        return false;

    m_classes[name] = _class;
    return true;
}

const char *MirRegisterBank::getName() const { return m_name; }

MirRegisterClass *MirRegisterBank::getClass(const std::string_view &name) const
{
    auto it = m_classes.find(name);

    if (it == m_classes.end())
        return nullptr;

    return it->second;
}

const std::pmr::unordered_map<std::string_view, MirRegisterClass *> &MirRegisterBank::getClasses() const
{
    return m_classes;
}