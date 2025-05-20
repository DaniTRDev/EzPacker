#ifndef EZPACKER_IBIGNUMBER_H
#define EZPACKER_IBIGNUMBER_H

#include "EzFrontendCommon.h"

/**
 * Interface used to defined the most basic operations of a big number.
 */
class IBigNumber
{
  public:
    virtual ~IBigNumber() = default;

    /**
     * Parses the given string into an integer. Returns true if succeeded.
     * @param str
     * @return bool
     */
    virtual bool fromStr(const std::string &str) = 0;

    /**
     * Returns the size in bits of the number.
     * @return size_t
     */
    virtual size_t getBitSize() = 0;
    
    /**
     * Returns the number in a formatted string.
     * @return std::string
     */
    virtual std::string toString() = 0;
};

#endif // EZPACKER_IBIGNUMBER_H
