#ifndef EZPACKER_STRINGUTILS_H
#define EZPACKER_STRINGUTILS_H

#include "EzCoreCommon.h"

inline std::string StrToLower(const std::string &str)
{
    std::string lowerStr;
    lowerStr.resize(str.size());

    std::transform(str.begin(), str.end(), lowerStr.begin(), tolower);
    return lowerStr;
}

inline std::string StrToUpper(const std::string &str)
{
    std::string upperStr = str;
    upperStr.resize(str.size());

    std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), toupper);
    return upperStr;
}

#endif // EZPACKER_STRINGUTILS_H
