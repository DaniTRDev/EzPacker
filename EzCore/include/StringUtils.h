#ifndef EZCORE_STRING_UTILS_H
#define EZCORE_STRING_UTILS_H

#include "EzCoreCommon.h"

/**
 * Converts all characters in a given string view to lowercase using ASCII transformations.
 * Allocates and returns a new std::string of the same length with lowercase characters.
 */
inline std::string StrToLower(const std::string_view &str)
{
    std::string lowerStr;
    lowerStr.resize(str.size());

    std::transform(str.begin(), str.end(), lowerStr.begin(), tolower);
    return lowerStr;
}

/**
 * Converts all characters in a given string view to uppercase using ASCII transformations.
 * Allocates and returns a new std::string of the same length with uppercase characters.
 */
inline std::string StrToUpper(const std::string_view &str)
{
    std::string upperStr;
    upperStr.resize(str.size());

    std::transform(str.begin(), str.end(), upperStr.begin(), toupper);
    return upperStr;
}

#endif // EZCORE_STRING_UTILS_H
