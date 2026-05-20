#ifndef EZPACKER_MATHUTILS_H
#define EZPACKER_MATHUTILS_H

#include "EzCoreCommon.h"

namespace MathUtils
{

/**
 * @brief Aligns the given value to the specified alignment.
 * @param value Value to align.
 * @param alignment Alignment boundary.
 * @returns The aligned value.
 */
template <typename T> inline T alignTo(T value, T alignment)
{
    if (value % alignment == 0)
        return value;

    return value + (alignment - (value % alignment));
}

/**
 * @brief Checks if the given value is aligned to the specified alignment.
 * @param value Value to check.
 * @param alignment Alignment boundary.
 * @returns True if aligned, false otherwise.
 */
template <typename T> inline bool isAligned(T value, T alignment) { return value % alignment == 0; }

/**
 * @brief Grows the given address by the specified size, aligning it to the specified alignment.
 * @param address Base address.
 * @param size Size to add or subtract.
 * @param alignment Alignment boundary.
 * @param upwards If true, the address is increased; otherwise, decreased.
 * @returns The new aligned address.
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
