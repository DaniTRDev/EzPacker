#ifndef EZCORE_COMMON_STD_H
#define EZCORE_COMMON_STD_H

/**
 * Shared standard-library include base for the EzPacker subsystems.
 *
 * Each component's precompiled "common" header (EzDslLexerCommon.h,
 * EzDslSemaCommon.h, EzDslCodeGeneratorsCommon.h, EzDslCliCommon.h, ...) pulls in
 * this file and only adds its component-specific extras, so the common std
 * include block lives in one place instead of drifting between copies.
 * StringUtils is included here as well: every component that needs the shared
 * string/identifier/escape helpers gets them through this base.
 */

#include "StringUtils.h"

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory_resource>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#endif // EZCORE_COMMON_STD_H
