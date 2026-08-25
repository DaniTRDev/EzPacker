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
#include <cmath>

#include <tommath.h>

extern "C"
{
    namespace libbf
    {
#include <libbf.h>
    };
};

#include <EzLogger.h>

inline std::unique_ptr<SyncLogger> g_logger = []() { return EzLogger::createSyncLogger("EzPacker"); }();

#endif // EZCORE_COMMON_H
