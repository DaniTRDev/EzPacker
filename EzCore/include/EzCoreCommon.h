#ifndef EZCORE_COMMON_H
#define EZCORE_COMMON_H

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <list>
#include <memory_resource>
#include <cmath>

#include <tommath.h>

#include "LibBFWrapper.h"

#include <EzLogger.h>

/**
 * Global synchronous logger instance used across all EzPacker compiler subsystems.
 * Initialized eagerly via an immediately invoked lambda expression (IIFE) using
 * EzLogger's synchronous logger creation factory with channel name "EzPacker".
 */
inline std::unique_ptr<SyncLogger> g_logger = []() { return EzLogger::createSyncLogger("EzPacker"); }();

#endif // EZCORE_COMMON_H
