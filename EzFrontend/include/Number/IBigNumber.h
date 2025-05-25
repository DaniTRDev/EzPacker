#ifndef EZPACKER_IBIGNUMBER_H
#define EZPACKER_IBIGNUMBER_H

#include "EzFrontendCommon.h"

/**
 * FloatingPoint and Double should be moved in a future to their own "FloatingPoint" that wraps handling very big
 * dot values.
 */
enum class BigNumberType : uint8_t
{
    Invalid,
    Int,
    Float,
    Double
};

/**
 * Interface used to define the most basic operations of a big number.
 */
class IBigNumber
{
  public:
    virtual ~IBigNumber() = default;

    /**
     * Returns the type of the number.
     * @return BigNumberType
     */
    virtual BigNumberType getType() = 0;

    /**
     * Parses the given string into an integer. Returns true if succeeded.
     * @param str
     * @return bool
     */
    virtual bool fromStr(const std::string &str) = 0;

    /**
     * Returns the encoded (raw binary bits) of the number.
     * @return char*
     */
    virtual char *getEncoded() = 0;

    /**
     * Returns the size in bits of the number.
     * @return size_t
     */
    virtual size_t getBitSize() = 0;

    /**
     * Returns the size of the encoded buffer.
     * @return size_t
     */
    virtual size_t getEncodedSize() = 0;

    /**
     * Returns the number in a formatted string.
     * @return std::string
     */
    virtual std::string toString() = 0;
};

#endif // EZPACKER_IBIGNUMBER_H
