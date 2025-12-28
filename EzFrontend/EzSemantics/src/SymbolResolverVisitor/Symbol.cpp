#include "SymbolResolverVisitor/Symbol.h"

Symbol::Symbol(size_t id, SymbolType symbolType, std::string name, std::string symbolDataType) :
    m_id(id), m_symbolType(symbolType), m_name(std::move(name)), m_symbolDataType(std::move(symbolDataType))
{
}

const char *Symbol::getSymbolTypeName() const
{
    switch (m_symbolType)
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

size_t Symbol::getId() const { return m_id; }

SymbolType Symbol::getType() const { return m_symbolType; }

const std::string &Symbol::getName() const { return m_name; }

const std::string &Symbol::getSymbolDataType() const { return m_symbolDataType; }
