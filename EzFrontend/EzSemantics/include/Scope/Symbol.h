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
    Module,
    ModuleParameter
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
     * @param symbolDataType
     * @param symbolType
     * @param definingNode
     * @param name
     */
    Symbol(SymbolType symbolType,
           const std::shared_ptr<AstNode> &definingNode,
           const std::shared_ptr<Type> &symbolDataType,
           const std::string &name);

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
     * Sets the ID of this symbol. Called by the scope when a new symbol, is created.
     * @param id
     */
    void setId(size_t id);

    /**
     * Returns the AstNode that defined this symbol.
     * @return const std::shared_ptr<AstNode> &
     */
    const std::shared_ptr<AstNode> &getDefiningNode() const;

    /**
     * Returns the data type behind this symbol.
     * @return SymbolDataType
     */
    const std::shared_ptr<Type> &getSymbolDataType() const;

    /**
     * Returns the name of the symbol.
     * @return const std::string &
     */
    const std::string &getName() const;

  private:
    size_t m_id;
    SymbolType m_symbolType;
    std::shared_ptr<AstNode> m_definingNode; // Where this symbol was firstly defined.
    std::shared_ptr<Type> m_symbolDataType;  // i64, i32, ...
    std::string m_name;
};

#endif // EZPACKER_SYMBOL_H
