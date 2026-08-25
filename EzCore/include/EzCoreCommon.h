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
    /**
     * LibBF arbitrary-precision floating-point library C bindings.
     * Isolated within the libbf namespace to prevent symbol collisions with standard math symbols.
     */
    namespace libbf
    {
#include <libbf.h>
    };
};

#include <EzLogger.h>

/**
 * Global synchronous logger instance used across all EzPacker compiler subsystems.
 * Initialized eagerly via an immediately invoked lambda expression (IIFE) using
 * EzLogger's synchronous logger creation factory with channel name "EzPacker".
 */
inline std::unique_ptr<SyncLogger> g_logger = []() { return EzLogger::createSyncLogger("EzPacker"); }();

#endif // EZCORE_COMMON_H
