#ifndef EZCORE_STRING_UTILS_H
#define EZCORE_STRING_UTILS_H

#include "EzCoreCommon.h"
#include <algorithm>
#include <cctype>

/**
 * Converts all characters in a given string view to lowercase using ASCII transformations.
 * Allocates and returns a new std::string of the same length with lowercase characters.
 */
inline std::string StrToLower(std::string_view str)
{
    std::string lowerStr;
    lowerStr.resize(str.size());

    // Cast through unsigned char: passing a negative char to tolower is undefined behavior.
    std::transform(str.begin(), str.end(), lowerStr.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lowerStr;
}

/**
 * Converts all characters in a given string view to uppercase using ASCII transformations.
 * Allocates and returns a new std::string of the same length with uppercase characters.
 */
inline std::string StrToUpper(std::string_view str)
{
    std::string upperStr;
    upperStr.resize(str.size());

    // Cast through unsigned char: passing a negative char to toupper is undefined behavior.
    std::transform(str.begin(), str.end(), upperStr.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return upperStr;
}

#endif // EZCORE_STRING_UTILS_H
