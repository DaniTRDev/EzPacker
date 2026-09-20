#ifndef EZTARGETS_X86_64_COMMON_H
#define EZTARGETS_X86_64_COMMON_H

// Precompiled/shared standard-library base for the x86-64 target library. It intentionally
// mirrors the union of the generic EzTriple and EzCodeEmitter common headers so the migrated
// target sources keep their original assumptions without depending on either project's PCH.

#include <bit>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iomanip>
#include <memory>
#include <memory_resource>
#include <set>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "EzMirCommon.h"

#endif // EZTARGETS_X86_64_COMMON_H
