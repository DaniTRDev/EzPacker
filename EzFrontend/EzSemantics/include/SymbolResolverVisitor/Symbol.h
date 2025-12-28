#ifndef EZPACKER_SYMBOL_H
#define EZPACKER_SYMBOL_H

#include "EzSemanticsCommon.h"

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
     * Creates a symbol with the given id, symbolType, name and data type.
     * @param id
     * @param symbolType
     * @param name
     * @param symbolDataType
     */
    Symbol(size_t id, SymbolType symbolType, std::string name, std::string symbolDataType);

    /**
     * Returns the name of the symbol type
     * @return const char *
     */
    const char *getSymbolTypeName() const;

    /**
     * Returns the id of the symbol.
     * @return size_t
     */
    size_t getId() const;

    /**
     * Returns the type of the symbol.
     * @return SymbolType
     */
    SymbolType getType() const;

    /**
     * Returns the name of the symbol.
     * @return const std::string &
     */
    const std::string &getName() const;

    /**
     * Returns the data type behind this symbol.
     * @return const std::string &
     */
    const std::string &getSymbolDataType() const;

  private:
    size_t m_id;
    SymbolType m_symbolType;
    std::string m_name;
    std::string m_symbolDataType; // i64, i32, ...
};

#endif // EZPACKER_SYMBOL_H
