#ifndef EZPACKER_SCOPEABLEANNOTATOR_H
#define EZPACKER_SCOPEABLEANNOTATOR_H

#include "EzAnnotatorCommon.h"
#include "INodeAnnotator.h"
#include "ScopedTable/ScopedType.h"
#include "ScopedTable/ScopedSymbol.h"

/**
 * This class is a helper to define annotators that requires information related to scopes. It contains the symbol
 * and type table.
 */
class ScopeAbleAnnotator : public INodeAnnotator
{
  public:
    using SymbolTableT = std::shared_ptr<ScopedTable<ScopedSymbol>>;
    using TypeTableT = std::shared_ptr<ScopedTable<ScopedType>>;

    /**
     * Sets the symbol table to the one given.
     * @param symbolTable
     */
    void setSymbolTable(const SymbolTableT &symbolTable);

    /**
     * Sets the type table to the one given.
     * @param typeTable
     */
    void setTypeTable(const TypeTableT &typeTable);

    /**
     * Returns the symbol table.
     * @return const SymbolTableT &
     */
    const SymbolTableT &getSymbolTable();

    /**
     * Returns the type table.
     * @return const TypeTableT &
     */
    const TypeTableT &getTypeTable();

  private:
    SymbolTableT m_symbolTable;
    TypeTableT m_typeTable;
};

#endif // EZPACKER_SCOPEABLEANNOTATOR_H
