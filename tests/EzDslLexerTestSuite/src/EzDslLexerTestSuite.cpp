#include "EzDslLexerTestSuite.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Parser/ParseContext.h"
#include "SourceManager/SourceManager.h"

// Retrieves the active diagnostic collector.
DiagnosticCollector *EzDslLexerTestSuite::getDiagCollector() { return m_diagnosticCollector; }

// Retrieves the diagnostic logger.
DiagnosticLogger *EzDslLexerTestSuite::getDiagLogger() { return m_diagnosticLogger; }

/**
 * Creates a ParseContext bound to an in-memory string buffer:
 * 1. Registers the buffer with the SourceManager under the given source name.
 * 2. Throws an exception if registration fails.
 * 3. Constructs and returns a ParseContext configured with the test allocator and diagnostics.
 */
ParseContext EzDslLexerTestSuite::createParseContextFromBuff(const std::string &sourceName,
                                                             const std::string &sourceContent)
{
    size_t sourceId = m_sourceManager->addSourceContent(sourceName, sourceContent);
    if (sourceId == 0)
    {
        throw std::runtime_error("Failed to create ParseContext because source was already added");
    }

    return ParseContext(m_diagnosticCollector, m_sourceManager, sourceId, &m_allocator);
}

// Retrieves the source manager.
SourceManager *EzDslLexerTestSuite::getSourceManager() { return m_sourceManager; }

/**
 * Initializes the test suite by allocating the diagnostic collector, source manager,
 * and diagnostic logger from the internal PMR buffer resource, enabling trace and debug logs.
 */
void EzDslLexerTestSuite::create()
{
    std::pmr::polymorphic_allocator<> alloc(&m_allocator);
    m_diagnosticCollector = alloc.new_object<DiagnosticCollector>();
    m_sourceManager = alloc.new_object<SourceManager>(std::filesystem::current_path(), &m_allocator);
    m_diagnosticLogger = alloc.new_object<DiagnosticLogger>(m_sourceManager);

    m_diagnosticCollector->addListener(m_diagnosticLogger);
    m_diagnosticCollector->enableDiag(Diag_Trace);
    m_diagnosticCollector->enableDiag(Diag_Debug);
}

/**
 * Destroys all allocated diagnostic and source manager objects.
 */
void EzDslLexerTestSuite::destroy()
{
    std::pmr::polymorphic_allocator<> alloc(&m_allocator);
    alloc.delete_object(m_sourceManager);
    alloc.delete_object(m_diagnosticCollector);
    alloc.delete_object(m_diagnosticLogger);
}

// Retrieves the monotonic memory resource.
std::pmr::memory_resource *EzDslLexerTestSuite::getAllocator() { return &m_allocator; }

// Sets up the test fixture by initializing EzDslLexerTestSuite.
void DslLexerTestSuiteAsGtest::SetUp()
{
    Test::SetUp();
    EzDslLexerTestSuite::create();
}

// Tears down the test fixture by destroying EzDslLexerTestSuite.
void DslLexerTestSuiteAsGtest::TearDown()
{
    EzDslLexerTestSuite::destroy();
    Test::TearDown();
}