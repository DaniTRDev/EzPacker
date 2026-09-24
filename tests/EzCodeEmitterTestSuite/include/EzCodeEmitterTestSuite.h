#ifndef EZCODEEMITTERTESTSUITE_H
#define EZCODEEMITTERTESTSUITE_H

#include "gtest/gtest.h"
#include "EzCodeEmitter.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "SourceManager/SourceManager.h"
#include "Type/MirTypeTable.h"
#include "Builder/MirBuilderContext.h"
#include <memory_resource>

/**
 * Base test fixture for EzCodeEmitter test suite.
 */
class EzCodeEmitterTestSuite : public ::testing::Test
{
  protected:
    void SetUp() override;
    void TearDown() override;

  public:
    std::pmr::memory_resource *getAllocator() { return &m_arena; }
    DiagnosticCollector *getDiagCollector() { return m_diagCollector.get(); }
    MirBuilderContext *getBuilderCtx() { return m_builderCtx.get(); }
    MirTypeTable *getTypeTable() { return m_typeTable.get(); }

  private:
    std::pmr::monotonic_buffer_resource m_arena;
    std::unique_ptr<DiagnosticCollector> m_diagCollector;
    std::unique_ptr<DiagnosticLogger> m_diagLogger;
    std::unique_ptr<SourceManager> m_sourceManager;
    std::unique_ptr<MirTypeTable> m_typeTable;
    std::unique_ptr<MirBuilderContext> m_builderCtx;
};

#endif // EZCODEEMITTERTESTSUITE_H
