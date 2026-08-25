#ifndef EZDSLTESTSUITE_EZ_DSL_TEST_SUITE_H
#define EZDSLTESTSUITE_EZ_DSL_TEST_SUITE_H

#include "EzDslCommon.h"
#include <gtest/gtest.h>

/**
 * Base test fixture and context manager for EzDSL compiler unit tests.
 * Manages monotonic buffer allocation, diagnostic reporting, source buffers,
 * and lexy-based DSL parsing contexts.
 */
class EzDslTestSuite
{
  public:
    /**
     * Returns the diagnostic collector for capturing warnings, errors, and traces.
     */
    class DiagnosticCollector *getDiagCollector();

    /**
     * Returns the diagnostic logger for formatting and outputting diagnostic messages.
     */
    class DiagnosticLogger *getDiagLogger();

    /**
     * Constructs a parse context from an in-memory source buffer and registers it with the source manager.
     */
    class ParseContext createParseContextFromBuff(const std::string &sourceName, const std::string &sourceContent);

    /**
     * Returns the source manager managing source files, buffers, and source locations.
     */
    class SourceManager *getSourceManager();

    /**
     * Initializes the DSL test suite context, allocator, diagnostic collectors, and source manager.
     */
    virtual void create();

    /**
     * Frees resources allocated for the DSL test suite context.
     */
    virtual void destroy();

    /**
     * Returns the PMR memory resource associated with this test suite.
     */
    std::pmr::memory_resource *getAllocator();

  private:
    class DiagnosticCollector *m_diagnosticCollector;
    class DiagnosticLogger *m_diagnosticLogger;
    class SourceManager *m_sourceManager;
    std::pmr::monotonic_buffer_resource m_allocator;
};

/**
 * GoogleTest fixture wrapper that manages the lifecycle of EzDslTestSuite in SetUp() and TearDown().
 */
class DslTestSuiteAsGtest : public EzDslTestSuite, public ::testing::Test
{
  public:
    /**
     * Initializes the test fixture before each test execution.
     */
    void SetUp() override;

    /**
     * Tears down and frees resources after each test execution.
     */
    void TearDown() override;
};

#endif // EZDSLTESTSUITE_EZ_DSL_TEST_SUITE_H
