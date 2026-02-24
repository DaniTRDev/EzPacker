#include "Scope/Symbol.h"

Symbol::Symbol(SymbolType symbolType,
               const std::shared_ptr<AstNode> &definingNode,
               const std::shared_ptr<Type> &symbolDataType,
               const std::string &name) :
    m_symbolType(symbolType), m_definingNode(definingNode), m_name(name), m_symbolDataType(symbolDataType)
{
}

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

void Symbol::setId(size_t id) { m_id = id; }

const std::shared_ptr<AstNode> &Symbol::getDefiningNode() const { return m_definingNode; }

const std::shared_ptr<Type> &Symbol::getSymbolDataType() const { return m_symbolDataType; }

const std::string &Symbol::getName() const { return m_name; }
