#ifndef EZPACKER_NORMALTABLE_H
#define EZPACKER_NORMALTABLE_H

#include "EzFrontendCommon.h"

/**
 * This class represents a table that will contain data. It contains a std::unordered_map<std::string, size_t> that
 * maps given "non-normal" value to a "normal" value (a numerical value really).
 */
class NormalTable
{
  public:
    /**
     * Creates the object.
     */
    NormalTable();
    
    /**
     * Destroys the object and free resources.
     */
    ~NormalTable();
    
    /**
     * Tries to add an element to the table, if it didn't exist previously.
     * @param elem
     * @return bool
     */
    bool addElement(const std::string &elem);
    
    /**
     * Returns true if the element exists in the table.
     * @param elem
     * @return bool
     */
    bool doesElemExist(const std::string &elem) const;
    
    /**
     * Returns the id of the element, starting at one. If an error occurred, 0 is returned.
     * @param elem
     * @return size_t
     */
    size_t getElemId(const std::string &elem) const;
    
    /**
     * Clears the table.
     */
    void clear();
    
    /**
     * Returns the internal structure used to save table's content.
     * @return const std::map<std::string, size_t> &
     */
    const std::unordered_map<std::string, size_t> &getTable() const;
    
  private:
    std::unordered_map<std::string, size_t> m_table;
};

#endif // EZPACKER_NORMALTABLE_H
