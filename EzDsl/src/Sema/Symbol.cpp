#include "Sema/Symbol.h"

Symbol::Symbol(class SourceReference *sourceRef,
               SymbolFlags flags,
               SymbolId definingScope,
               SymbolId id,
               SymbolType type,
               std::string_view name) :
    m_sourceRef(sourceRef), m_flags(flags), m_definingScopeId(definingScope), m_id(id), m_type(type), m_name(name)
{
}

class SourceReference *Symbol::getSourceRef() const { return m_sourceRef; }

SymbolFlags Symbol::getFlags() const { return m_flags; }

SymbolId Symbol::getDefiningScopeId() const { return m_definingScopeId; }

SymbolId Symbol::getId() const { return m_id; }

SymbolType Symbol::getType() const { return m_type; }

const std::string_view &Symbol::getName() const { return m_name; }