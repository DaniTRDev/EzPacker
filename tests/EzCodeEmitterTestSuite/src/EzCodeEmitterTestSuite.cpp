#include "EzCodeEmitterTestSuite.h"
#include <filesystem>

void EzCodeEmitterTestSuite::SetUp()
{
    m_diagCollector = std::make_unique<DiagnosticCollector>();
    m_sourceManager = std::make_unique<SourceManager>(std::filesystem::current_path(), &m_arena);
    m_diagLogger = std::make_unique<DiagnosticLogger>(m_sourceManager.get());
    m_typeTable = std::make_unique<MirTypeTable>(&m_arena);
    m_typeTable->initialize(64);
    m_builderCtx = std::make_unique<MirBuilderContext>(nullptr, m_diagCollector.get(), m_typeTable.get(), &m_arena);

    m_diagCollector->addListener(m_diagLogger.get());
    m_diagCollector->enableDiag(Diag_Trace);
    m_diagCollector->enableDiag(Diag_Debug);
}

void EzCodeEmitterTestSuite::TearDown()
{
    m_diagLogger.reset();
    m_builderCtx.reset();
    m_typeTable.reset();
    m_diagCollector.reset();
    m_sourceManager.reset();
}
