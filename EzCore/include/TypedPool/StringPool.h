#ifndef EZPACKER_STRINGPOOL_H
#define EZPACKER_STRINGPOOL_H

#include "EzCoreCommon.h"
#include "TypedArrayPool.h"

class StringPool : public TypedArrayPool<char>
{
  public:
    /**
     * @brief Creates an empty (filled with 0s) string in the pool.
     * @param len Length of the string (MUST NOT INCLUDE NULL TERMINATOR).
     * @returns A string view of the allocated string.
     */
    std::string_view createConstantString(size_t len)
    {
        if (len == 0)
            return "";

        ConstantArray<char> result = createConstantArray(len);
        return std::string_view(result.m_elems, result.m_numElems);
    }

    /**
     * @brief Creates an empty (filled with 0s) string in the pool and fills it with the given string.
     * @param from The string to copy into the pool.
     * @returns A string view of the allocated string.
     */
    std::string_view createConstantString(const std::string &from)
    {
        if (from.empty())
            return "";

        std::string_view result = createConstantString(from.size());
        std::copy_n((char *)from.data(), from.size(), (char *)result.data()); // Safe to perform this copy.

        return std::move(result);
    }

  private:
};

#endif // EZPACKER_STRINGPOOL_H
