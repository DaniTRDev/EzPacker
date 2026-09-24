#include "Sema/Symbol.h"

Symbol::Symbol(
        class SourceReference *sourceRef, SymbolId definingScope, SymbolId id, SymbolType type, std::string_view name) :
    m_sourceRef(sourceRef), m_definingScopeId(definingScope), m_id(id), m_type(type), m_name(name)
{
}

class SourceReference *Symbol::getSourceRef() const { return m_sourceRef; }

SymbolId Symbol::getDefiningScopeId() const { return m_definingScopeId; }

SymbolId Symbol::getId() const { return m_id; }

SymbolType Symbol::getType() const { return m_type; }

void Symbol::setData(Symbol::SymbolData data) { m_data = std::move(data); }

const std::string_view &Symbol::getName() const { return m_name; }