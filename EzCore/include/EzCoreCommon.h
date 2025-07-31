#ifndef EZCORE_COMMON_H
#define EZCORE_COMMON_H
// EzCore Precompiled Header
#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>

#include <EzLogger.h>

inline std::unique_ptr<SyncLogger> g_logger = EzLogger::createSinkLogger("EzPacker");

#endif // EZCORE_COMMON_H
