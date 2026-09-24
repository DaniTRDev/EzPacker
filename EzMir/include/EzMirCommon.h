#ifndef EZMIR_EZ_MIR_COMMON_H
#define EZMIR_EZ_MIR_COMMON_H

#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <set>
#include <typeindex>
#include <unordered_set>
#include <iomanip>
#include <sstream>

#include "StringUtils.h"

/**
 * Unique identifier type used across all MIR entities (functions, blocks, instructions, operands, global variables).
 */
using MirId = size_t;

/**
 * Sentinel value indicating an invalid or unassigned MIR identifier.
 */
constexpr MirId MIRID_INVALID = 0;

/**
 * Identifier type for physical machine registers mapped during target lowering and register allocation.
 */
using MirPhysicalRegId = size_t;

#endif // EZMIR_EZ_MIR_COMMON_H
