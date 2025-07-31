#include "NodeAnnotators/ScopeAbleAnnotator.h"

void ScopeAbleAnnotator::setSymbolTable(const ScopeAbleAnnotator::SymbolTableT &symbolTable)
{
    m_symbolTable = symbolTable;
}

void ScopeAbleAnnotator::setTypeTable(const ScopeAbleAnnotator::TypeTableT &typeTable) { m_typeTable = typeTable; }

const ScopeAbleAnnotator::SymbolTableT &ScopeAbleAnnotator::getSymbolTable() { return m_symbolTable; }

const ScopeAbleAnnotator::TypeTableT &ScopeAbleAnnotator::getTypeTable() { return m_typeTable; }
