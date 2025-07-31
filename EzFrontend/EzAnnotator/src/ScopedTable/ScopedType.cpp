#include "ScopedTable/ScopedType.h"

ScopedType::ScopedType(const std::string &name) : m_name(name), m_string(getStringFromName(m_name)) {}

const std::string &ScopedType::getName() { return m_name; }

const std::string &ScopedType::getString() { return m_string; }

std::string ScopedType::getStringFromName(const std::string &name) { return std::string("@Type@") + name; }
