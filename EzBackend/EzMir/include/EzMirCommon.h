/**
 * @file EzMirCommon.h
 * @brief Shared foundational includes for the EzMir library.
 *
 * This header centralizes the standard-library, third-party, and EzCore
 * includes needed across EzMir. The module uses it as its precompiled header,
 * so most public/private EzMir files include it either directly or
 * transitively.
 */
#ifndef EZPACKER_EZMIRCOMMON_H
#define EZPACKER_EZMIRCOMMON_H

#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <set>
#include <typeindex>
#include <unordered_set>

#include <tommath.h>
#include <EzCore.h>

using MirId = size_t;
constexpr MirId MIRID_INVALID = 0;

#endif // EZPACKER_EZMIRCOMMON_H
