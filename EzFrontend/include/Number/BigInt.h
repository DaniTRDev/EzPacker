#ifndef EZPACKER_BIGINT_H
#define EZPACKER_BIGINT_H

#include "EzFrontendCommon.h"
#include "IBigNumber.h"

#define LOG2(number)                                                                                                   \
    [](size_t num) -> size_t {                                                                                         \
        size_t result = 1;                                                                                             \
        while (num >>= 1)                                                                                              \
            result++;                                                                                                  \
        return result;                                                                                                 \
    }(number);

class BigInt : public IBigNumber
{
  public:
    using BlockSizeT = uint32_t;
    size_t BlockSize = sizeof(BlockSizeT);

    /**
     * Creates the object.
     */
    BigInt();

    /**
     * Returns the type of the number.
     * @return BigNumberType
     */
    BigNumberType getType() override;

    /**
     * Parses the given string into an integer. Returns true if succeeded. It assumes str is in base-10
     * if it isn't and there's an invalid digit, it will throw an exception.
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
    /**
     * Returns the digit for the given character. It ASSUMES base-10.
     * @param ch
     * @return uint8_t
     */
    uint8_t digitFromStr(uint8_t ch);

    /**
     * Returns the string character of a digit.
     * @param ch
     * @return uint8_t
     */
    uint8_t digitToStr(uint8_t ch);

  private:
    std::vector<BlockSizeT> m_blocks;
};

#endif // EZPACKER_BIGINT_H
