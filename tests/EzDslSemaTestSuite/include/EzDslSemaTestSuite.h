#ifndef EZDSLSEMATESTSUITE_EZ_DSL_SEMA_TEST_SUITE_H
#define EZDSLSEMATESTSUITE_EZ_DSL_SEMA_TEST_SUITE_H

#include "gtest/gtest.h"
#include "Parser/ParseContext.h"

/**
 * Forward declarations.
 */
namespace DSL::Ast::TypeDef
{
enum class TypeKind : uint8_t;
}; // namespace DSL::Ast::TypeDef

class EzDslSemaTestSuite
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
     * Returns the symbol table.
     */
    class SymbolTable *getSymbolTable();

    /**
     * Initializes the DSL test suite context, allocator, diagnostic collectors, and source manager.
     */
    virtual void create();

    /**
     * Frees resources allocated for the DSL test suite context.
     */
    virtual void destroy();

    /**
     * Registers a type with the given name and bitwidth.
     */
    void registerType(std::string_view name,
                      DSL::Ast::TypeDef::TypeKind kind,
                      uint32_t bitWidth,
                      uint32_t alignment,
                      uint8_t compactId);

    /**
     * Returns the PMR memory resource associated with this test suite.
     */
    std::pmr::memory_resource *getAllocator();

  private:
    class DiagnosticCollector *m_diagnosticCollector;
    class DiagnosticLogger *m_diagnosticLogger;
    class SourceManager *m_sourceManager;
    class SymbolTable *m_symbolTable;
    std::pmr::monotonic_buffer_resource m_allocator;
};

/**
 * GoogleTest fixture wrapper that manages the lifecycle of EzDslLexerTestSuite in SetUp() and TearDown().
 */
class EzDslSemaTestSuiteAsGtest : public EzDslSemaTestSuite, public ::testing::Test
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

#endif // EZDSLSEMATESTSUITE_EZ_DSL_SEMA_TEST_SUITE_H
