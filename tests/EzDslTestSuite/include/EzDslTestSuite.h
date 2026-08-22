#ifndef EZDSLTESTSUITE_EZ_DSL_TEST_SUITE_H
#define EZDSLTESTSUITE_EZ_DSL_TEST_SUITE_H

#include "EzDslCommon.h"
#include <gtest/gtest.h>

class EzDslTestSuite
{
  public:
    /**
     * Returns the diagnostic collector of this test suite.
     */
    class DiagnosticCollector *getDiagCollector();

    /**
     * Returns the diagnostic logger of this test suite.
     */
    class DiagnosticLogger *getDiagLogger();

    /**
     * Creates and returns a parse context for the given input buffer.
     */
    class ParseContext createParseContextFromBuff(const std::string &sourceName, const std::string &sourceContent);

    /**
     * Returns the source manager of this test suite.
     */
    class SourceManager *getSourceManager();

    /**
     * Creates the test suite. This function MUST always be called by derived types.
     */
    virtual void create();

    /**
     * Destroys the test suite. This function MUST always be called by derived types.
     */
    virtual void destroy();

    /**
     * Returns the allocator of the test suite.
     */
    std::pmr::memory_resource *getAllocator();

  private:
    class DiagnosticCollector *m_diagnosticCollector;
    class DiagnosticLogger *m_diagnosticLogger;
    class SourceManager *m_sourceManager;
    std::pmr::monotonic_buffer_resource m_allocator;
};

class DslTestSuiteAsGtest : public EzDslTestSuite, public ::testing::Test
{
  public:
    /**
     * Calls EzMirTestSuite::create.
     */
    void SetUp() override;

    /**
     * Calls EzMirTestSuite::destroy.
     */
    void TearDown() override;
};

#endif // EZDSLTESTSUITE_EZ_DSL_TEST_SUITE_H
