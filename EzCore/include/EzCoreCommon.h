#ifndef EZCORE_COMMON_H
#define EZCORE_COMMON_H

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <stack>
#include <functional>
#include <memory>
#include <list>
#include <memory_resource>

#include <tommath.h>
#include <tomfloat.h>

#include <EzLogger.h>

inline std::unique_ptr<SyncLogger> g_logger = EzLogger::createSyncLogger("EzPacker");

#endif // EZCORE_COMMON_H
