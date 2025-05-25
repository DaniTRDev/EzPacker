#pragma once

#include <functional>
#include <list>
#include <limits>
#include <set>
#include <stack>

#include <EzLibCommon.h>
#include <EzLogger.h>

inline std::unique_ptr<SyncLogger> g_logger = EzLogger::createSinkLogger("FRONTEND");