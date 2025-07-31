#ifndef EZPACKER_SCOPEDSYMBOL_H
#define EZPACKER_SCOPEDSYMBOL_H

#include "EzAnnotatorCommon.h"
#include "ScopedTable.h"

/**
 * Class that holds information about a symbol and that is dependant of a scope. It contains: its ID, name and string
 * (used to generate an ID). It also has a method to get the string from a name.
 */
class ScopedSymbol : public IndexableScopedItem
{
  public:
    /**
     * Creates the type with the given typeId and name.
     * @param typeId
     * @param name
     */
    ScopedSymbol(size_t typeId, const std::string &name);

    /**
     * Returns the ID of the type of this symbol.
     * @return size_t
     */
    size_t getTypeId();

    /**
     * Returns the name of the symbol.
     * @return const std::string &
     */
    const std::string &getName();

    /**
     * Returns "@Symbol@{SymbolName}"
     * @return const std::string &
     */
    const std::string &getString() override;

    /**
     * Returns the symbol string for the given type name.
     * @return std::string
     */
    static std::string getStringFromName(const std::string &name);

  private:
    size_t m_typeId;
    std::string m_name;
    std::string m_string; // What's going to be returned in getString.
};

#endif // EZPACKER_SCOPEDSYMBOL_H
