#ifndef EZPACKER_DOUBLE_H
#define EZPACKER_DOUBLE_H

#include "EzFrontendCommon.h"
#include "IBigNumber.h"

/**
 * This class wraps double type so it can be read / written easily.
 */
class Double : public IBigNumber
{
  public:
    /**
     * Returns the type of the number.
     * @return BigNumberType
     */
    BigNumberType getType() override;

    /**
     * Parses the given string into an integer. Returns true if succeeded.
     * @param str
     * @return bool
     */
    bool fromStr(const std::string &str) override;

    /**
     * Returns the encoded (raw binary bits) of the number. This returns a pointer to the internal buffer,
     * modifying it WILL modify the number.
     * @return char*
     */
    char *getEncoded() override;

    /**
     * Returns the size in bits of the number.
     * @return size_t
     */
    size_t getBitSize() override;

    /**
     * Returns the size of the encoded buffer.
     * @return size_t
     */
    size_t getEncodedSize() override;

    /**
     * Returns the number in a formatted string.
     * @return std::string
     */
    std::string toString() override;

  private:
    double m_value;
};

#endif // EZPACKER_DOUBLE_H
