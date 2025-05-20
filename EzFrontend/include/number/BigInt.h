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
     * Parses the given string into an integer. Returns true if succeeded.
     * @param str
     * @return bool
     */
    bool fromStr(const std::string &str) override;

    /**
     * Returns the size in bits of the number.
     * @return size_t
     */
    size_t getBitSize() override;

    /**
     * Returns the number in a formatted string.
     * @return std::string
     */
    std::string toString() override;

  private:
    /**
     * Returns the digit for the given character.
     * @param ch
     * @param base
     * @return
     */
    uint8_t digitFromStr(uint8_t ch, size_t base);
    
  private:
    std::vector<BlockSizeT> m_blocks;
};

#endif // EZPACKER_BIGINT_H
