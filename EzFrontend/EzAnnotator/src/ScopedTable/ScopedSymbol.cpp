#include "ScopedTable/ScopedSymbol.h"

ScopedSymbol::ScopedSymbol(size_t typeId, const std::string &name) :
    m_typeId(typeId), m_name(name), m_string(getStringFromName(name))
{
}

size_t ScopedSymbol::getTypeId() { return m_typeId; }

const std::string &ScopedSymbol::getName() { return m_name; }

const std::string &ScopedSymbol::getString() { return m_string; }

std::string ScopedSymbol::getStringFromName(const std::string &name) { return std::string("@Symbol@") + name; }
