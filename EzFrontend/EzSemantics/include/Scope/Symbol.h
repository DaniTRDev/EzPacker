/**
 * @file Symbol.h
 * @brief Semantic description of a declared named entity.
 *
 * A `Symbol` represents one declaration that can later be referenced by
 * source code: modules, labels and variables. Symbols are created by
 * `BasicSemanticContext::createSymbol()` during the definition pass and then
 * referenced from AST nodes through `SymbolAnnotation` or
 * `TypeCastAnnotation`.
 *
 * The object stores:
 *   - the AST node that introduced the declaration,
 *   - the semantic category (`SymbolType`),
 *   - the declared data type when applicable,
 *   - a context-wide unique ID, and
 *   - the original source-level name.
 */
#ifndef EZPACKER_SYMBOL_H
#define EZPACKER_SYMBOL_H

#include "EzSemanticsCommon.h"
#include "TypeTable.h"

/**
 * Enumerates the kinds of source declarations represented in the symbol
 * table.
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
 * Semantic record for one declaration in the symbol table.
 */
class Symbol
{
  public:
    /**
     * Creates a symbol for one declaration site.
     */
    Symbol(AstNode *definingNode, Type *symbolDataType, SymbolType symbolType, const std::string_view &name);

    /**
     * Returns the AST node that introduced this symbol.
     */
    AstNode *getDefiningNode() const;

    /**
     * Returns a textual name for this symbol's semantic category.
     */
    const char *getSymbolTypeName() const;
    
    /**
     * Converts a `SymbolType` enum value to its human-readable string name.
     */
    static const char *getSymbolTypeAsString(SymbolType symbolType);
    
    /**
     * Returns the context-wide unique ID assigned to this symbol.
     */
    size_t getId() const;
    
    /**
     * Returns the semantic category of this symbol.
     */
    SymbolType getType() const;
    
    /**
     * Returns the declared data type associated with this symbol, if any.
     */
    Type *getSymbolDataType();
    
    /**
     * Sets the unique ID assigned by the semantic context.
     */
    void setId(size_t id);
    
    /**
     * Returns the source-level name of the symbol.
     */
    const std::string_view &getName() const;
    
    /**
     * Returns a textual name for the symbol's declared data type.
     *
     * This is meaningful only when the symbol has an associated type.
     */
    std::string_view getSymbolDataTypeName() const;

  private:
    AstNode *m_definingNode; // Where this symbol was firstly defined.
    SymbolType m_symbolType;
    size_t m_id;
    Type *m_symbolDataType; // i64, i32, ...
    std::string_view m_name;
};

#endif // EZPACKER_SYMBOL_H
