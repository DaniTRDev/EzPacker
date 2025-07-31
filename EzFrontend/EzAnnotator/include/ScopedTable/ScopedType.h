#ifndef EZPACKER_SCOPEDTYPE_H
#define EZPACKER_SCOPEDTYPE_H

#include "EzAnnotatorCommon.h"
#include "ScopedTable.h"

/**
 * Class that holds information about a type and that is dependant of a scope. It contains: its ID, name and string
 * (used to generate an ID). It also has a method to get the string from a name.
 */
class ScopedType : public IndexableScopedItem
{
  public:
    /**
     * Creates the type with the given name.
     * @param name
     */
    ScopedType(const std::string &name);

    /**
     * Returns the name of the type.
     * @return const std::string &
     */
    const std::string &getName();

    /**
     * Returns "@Type@{TypeName}"
     * @return const std::string &
     */
    const std::string &getString() override;

    /**
     * Returns the type string for the given type name.
     * @return std::string
     */
    static std::string getStringFromName(const std::string &name);

  private:
    std::string m_name;
    std::string m_string; // What's going to be returned in getString.
};

#endif // EZPACKER_SCOPEDTYPE_H
