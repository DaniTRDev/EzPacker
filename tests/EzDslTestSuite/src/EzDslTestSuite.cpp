#include "EzDslTestSuite.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Parser/ParseContext.h"
#include "SourceManager/SourceManager.h"

DiagnosticCollector *EzDslTestSuite::getDiagCollector() { return m_diagnosticCollector; }

DiagnosticLogger *EzDslTestSuite::getDiagLogger() { return m_diagnosticLogger; }

ParseContext EzDslTestSuite::createParseContextFromBuff(const std::string &sourceName, const std::string &sourceContent)
{
    size_t sourceId = m_sourceManager->addSourceContent(sourceName, sourceContent);
    if (sourceId == 0)
    {
        throw std::runtime_error("Failed to create ParseContext because source was already added");
    }

    return ParseContext(m_diagnosticCollector, m_sourceManager, sourceId, &m_allocator);
}

SourceManager *EzDslTestSuite::getSourceManager() { return m_sourceManager; }

void EzDslTestSuite::create()
{
    std::pmr::polymorphic_allocator<> alloc(&m_allocator);
    m_diagnosticCollector = alloc.new_object<DiagnosticCollector>();
    m_sourceManager = alloc.new_object<SourceManager>(std::filesystem::current_path(), &m_allocator);
    m_diagnosticLogger = alloc.new_object<DiagnosticLogger>(m_sourceManager);
}

void EzDslTestSuite::destroy()
{
    std::pmr::polymorphic_allocator<> alloc(&m_allocator);
    alloc.delete_object(m_sourceManager);
    alloc.delete_object(m_diagnosticCollector);
    alloc.delete_object(m_diagnosticLogger);
}

std::pmr::memory_resource *EzDslTestSuite::getAllocator() { return &m_allocator; }

void DslTestSuiteAsGtest::SetUp()
{
    Test::SetUp();
    EzDslTestSuite::create();
}

void DslTestSuiteAsGtest::TearDown()
{
    EzDslTestSuite::destroy();
    Test::TearDown();
}