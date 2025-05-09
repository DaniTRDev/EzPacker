#ifndef EZPACKER_EZIRBUILDERCOMMON_H
#define EZPACKER_EZIRBUILDERCOMMON_H

#include <functional>
#include <set>

#include <EzLibCommon.h>
#include <EzLogger.h>

inline std::unique_ptr<SyncLogger> g_logger = EzLogger::createSinkLogger("EZIRBUILDER");

#endif // EZPACKER_EZIRBUILDERCOMMON_H
