/**
 * @file Symbol.h
 * @brief A named entity in the symbol table: variable, label, or module.
 *
 * Each Symbol records the AstNode that originally defined it, the symbol's
 * kind (GlobalVariable, LocalVariable, Label, Module), its data type, a
 * unique numeric ID, and its name.  Symbols are created by
 * BasicSemanticContext::createSymbol() and stored in the Scope that owns
 * them.
 */
#ifndef EZPACKER_SYMBOL_H
#define EZPACKER_SYMBOL_H

#include "EzSemanticsCommon.h"
#include "TypeTable.h"

/**
 * This enumeration contains the possible type of a symbol.
 */
enum class SymbolType : uint8_t
{
    Invalid = 0,
    GlobalVariable,
    Label,
    LocalVariable,
    Module
};

/**
 * This class contains basic information about a symbol. It will be of use to define constraints during the semantic
 * analysis.
 */
class Symbol
{
  public:
    /**
     * Creates a symbol with the given data type, symbol type, defining node and name.
     * @param definingNode
     * @param symbolDataType
     * @param symbolType
     * @param name
     */
    Symbol(AstNode *definingNode, Type *symbolDataType, SymbolType symbolType, const std::string_view &name);

    /**
     * Returns the AstNode that defined this symbol.
     * @return AstNode *
     */
    AstNode *getDefiningNode() const;

    /**
     * Returns the name of the symbol data type.
     * @return const char*
     */
    const char *getSymbolDataTypeName() const;

    /**
     * Returns the name of the symbol type
     * @return const char *
     */
    const char *getSymbolTypeName() const;

    /**
     * Returns the name of the given symbol type.
     * @param symbolType
     * @return const char *
     */
    static const char *getSymbolTypeAsString(SymbolType symbolType);

    /**
     * Returns the ID of this symbol.
     * @return size_t
     */
    size_t getId() const;

    /**
     * Returns the type of the symbol.
     * @return SymbolType
     */
    SymbolType getType() const;

    /**
     * Returns the data type behind this symbol.
     * @return Type
     */
    Type *getSymbolDataType();

    /**
     * Sets the ID of this symbol. Called by the scope when a new symbol, is created.
     * @param id
     */
    void setId(size_t id);

    /**
     * Returns the name of the symbol.
     * @return const std::string &
     */
    const std::string_view &getName() const;

  private:
    AstNode *m_definingNode; // Where this symbol was firstly defined.
    SymbolType m_symbolType;
    size_t m_id;
    Type *m_symbolDataType; // i64, i32, ...
    std::string_view m_name;
};

#endif // EZPACKER_SYMBOL_H
