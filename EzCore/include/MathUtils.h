#ifndef EZPACKER_MATHUTILS_H
#define EZPACKER_MATHUTILS_H

#include "EzCoreCommon.h"

namespace MathUtils
{
/**
 * Aligns the given value to the specified alignment. If the value is already aligned, it is returned as is.
 * @tparam T
 * @param value
 * @param alignment
 * @return
 */
template <typename T> inline T alignTo(T value, T alignment)
{
    if (value % alignment == 0)
        return value;

    return value + (alignment - (value % alignment));
}

/**
 * Checks if the given value is aligned to the specified alignment.
 * @tparam T
 * @param value
 * @param alignment
 * @return
 */
template <typename T> inline bool isAligned(T value, T alignment) { return value % alignment == 0; }

/**
 * Grows the given address by the specified size, aligning it to the specified alignment. If upwards is true, the
 * address is increased; otherwise, it is decreased.
 * @tparam T
 * @param address
 * @param size
 * @param alignment
 * @param upwards
 * @return
 */
template <typename T> inline T growAddress(T address, T size, T alignment, bool upwards = true)
{
    if (upwards)
        return alignTo(address + size, alignment);
    else
        return alignTo(address - size, alignment);
}
}; // namespace MathUtils

#endif // EZPACKER_MATHUTILS_H
