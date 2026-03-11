#include "Scope/Symbol.h"

Symbol::Symbol(AstNode *definingNode, Type *symbolDataType, SymbolType symbolType, const std::string_view &name) :
    m_definingNode(definingNode), m_symbolType(symbolType), m_id(0), m_symbolDataType(symbolDataType), m_name(name)
{
}

AstNode *Symbol::getDefiningNode() const { return m_definingNode; }

const char *Symbol::getSymbolTypeAsString(SymbolType symbolType)
{
    switch (symbolType)
    {
        case SymbolType::Invalid:
            return "InvalidSymbol";
        case SymbolType::GlobalVariable:
            return "GlobalVariableSymbol";
        case SymbolType::Label:
            return "LabelSymbol";
        case SymbolType::LocalVariable:
            return "LocalVariableSymbol";
        case SymbolType::Module:
            return "ModuleSymbol";
    }
    return nullptr;
}

const char *Symbol::getSymbolTypeName() const { return getSymbolTypeAsString(m_symbolType); }

size_t Symbol::getId() const { return m_id; }

SymbolType Symbol::getType() const { return m_symbolType; }

Type *Symbol::getSymbolDataType() { return m_symbolDataType; }

void Symbol::setId(size_t id) { m_id = id; }

const std::string_view &Symbol::getName() const { return m_name; }

std::string_view Symbol::getSymbolDataTypeName() const
{
    if (m_symbolDataType)
        return m_symbolDataType->getTypeName();

    return "";
}
