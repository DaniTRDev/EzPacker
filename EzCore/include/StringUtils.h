#ifndef EZCORE_STRING_UTILS_H
#define EZCORE_STRING_UTILS_H

#include "EzCoreCommon.h"

/**
 * Converts a given string to lowercase.
 */
inline std::string StrToLower(const std::string_view &str)
{
    std::string lowerStr;
    lowerStr.resize(str.size());

    std::transform(str.begin(), str.end(), lowerStr.begin(), tolower);
    return lowerStr;
}

/**
 * Converts a given string to uppercase.
 */
inline std::string StrToUpper(const std::string_view &str)
{
    std::string upperStr;
    upperStr.resize(str.size());

    std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), toupper);
    return upperStr;
}

#endif // EZCORE_STRING_UTILS_H
