#ifndef EZDSLCODEGENERATORSTESTSUITE_EZ_DSL_CODE_GENERATORS_TEST_SUITE_H
#define EZDSLCODEGENERATORSTESTSUITE_EZ_DSL_CODE_GENERATORS_TEST_SUITE_H

#include <gtest/gtest.h>
#include "Ast/IrInstructionDefLangAst.h"
#include "Ast/TypeDefLangAst.h"
#include "Parser/ParseContext.h"

#include <filesystem>
#include <memory_resource>
#include <optional>
#include <string>
#include <string_view>

namespace DSL::Ast::IrInstDef
{
constexpr IrInstFlag operator|(IrInstFlag a, IrInstFlag b) noexcept
{
    return static_cast<IrInstFlag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
} // namespace DSL::Ast::IrInstDef

/**
 * Base test suite fixture managing EzDSL semantic and code generation resources.
 */
class EzDslCodeGeneratorsTestSuite
{
  public:
    virtual ~EzDslCodeGeneratorsTestSuite() = default;

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
     * Registers a type with the given name and bitwidth into the symbol table.
     */
    void registerType(std::string_view name,
                      DSL::Ast::TypeDef::TypeKind kind,
                      uint32_t bitWidth,
                      uint32_t alignment,
                      uint8_t compactId);

    /**
     * Parses an IR instruction definition DSL source string into an AST.
     */
    std::optional<DSL::Ast::IrInstDef::IrInstDefFile> parseIrInstDefFile(const std::string &sourceContent);

    /**
     * Parses a type definition DSL source string into an AST.
     */
    std::optional<DSL::Ast::TypeDef::TypeDefFile> parseTypeDefFile(const std::string &sourceContent);

    /**
     * Reads the entire content of a file on disk into a std::string.
     */
    static std::string readFileContent(const std::filesystem::path &filePath);

    /**
     * Returns the PMR memory resource associated with this test suite.
     */
    std::pmr::memory_resource *getAllocator();

  private:
    class DiagnosticCollector *m_diagnosticCollector{ nullptr };
    class DiagnosticLogger *m_diagnosticLogger{ nullptr };
    class SourceManager *m_sourceManager{ nullptr };
    class SymbolTable *m_symbolTable{ nullptr };
    std::pmr::monotonic_buffer_resource m_allocator;
    size_t m_sourceCounter{ 0 };
};

/**
 * GoogleTest fixture wrapper that manages the lifecycle of EzDslCodeGeneratorsTestSuite in SetUp() and TearDown(),
 * and provides a clean temporary filesystem sandbox directory for generated code output.
 */
class EzDslCodeGeneratorsTestSuiteAsGtest : public EzDslCodeGeneratorsTestSuite, public ::testing::Test
{
  public:
    /**
     * Initializes the test fixture and allocates an isolated temp directory before each test execution.
     */
    void SetUp() override;

    /**
     * Cleans up the temporary directory and tears down resources after each test execution.
     */
    void TearDown() override;

    /**
     * Returns the isolated temporary directory created for this test instance.
     */
    const std::filesystem::path &getTempDir() const noexcept { return m_testTempDir; }

  protected:
    std::filesystem::path m_testTempDir;
    static inline size_t s_testCounter{ 0 };
};

#endif // EZDSLCODEGENERATORSTESTSUITE_EZ_DSL_CODE_GENERATORS_TEST_SUITE_H
