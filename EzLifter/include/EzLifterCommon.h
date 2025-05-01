#ifndef EZPACKER_EZLIFTERCOMMON_H
#define EZPACKER_EZLIFTERCOMMON_H

#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>

#include <EzLogger.h>
#include <Zydis/Zydis.h>

#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>

#include <gtest/gtest.h>

inline std::unique_ptr<SyncLogger> g_logger = EzLogger::createSinkLogger("EZLIFTER");

#endif // EZPACKER_EZLIFTERCOMMON_H
