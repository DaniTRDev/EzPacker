#ifndef EZDSLCLITESTSUITE_EZ_DSL_CLI_TEST_SUITE_H
#define EZDSLCLITESTSUITE_EZ_DSL_CLI_TEST_SUITE_H

#include <gtest/gtest.h>

#include "Cli/CommandLineOptions.h"
#include "Cli/Driver.h"
#include "Cli/InfoDumper.h"

#include "Ast/IrInstructionDefLangAst.h"
#include "Ast/LegalizeActionDefLangAst.h"
#include "Ast/LegalizeRuleDefLangAst.h"
#include "Ast/TypeDefLangAst.h"
#include "Parser/ParseContext.h"

#include <filesystem>
#include <memory_resource>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/**
 * Base test suite fixture managing EzDSL CLI testing resources, paths, and helpers.
 */
class EzDslCliTestSuite
{
  public:
    virtual ~EzDslCliTestSuite() = default;

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
     * Returns the PMR memory resource associated with this test suite.
     */
    std::pmr::memory_resource *getAllocator();

    /**
     * Reads the entire content of a file on disk into a std::string.
     */
    static std::string readFileContent(const std::filesystem::path &filePath);

    /**
     * Writes the given content to a file on disk.
     */
    static bool writeFileContent(const std::filesystem::path &filePath, const std::string &content);

    /**
     * Locates the root repository directory of EzPacker.
     */
    static std::filesystem::path findEzPackerRoot();

    /**
     * Returns the absolute path to EzMir/types.tyf.
     */
    static std::filesystem::path getEzMirTypesPath();

    /**
     * Returns the absolute path to EzMir/instructions.irdf.
     */
    static std::filesystem::path getEzMirInstructionsPath();

    /**
     * Locates the compiled EzDslCli executable binary.
     */
    static std::filesystem::path findEzDslCliExe();

    /**
     * Helper to create a temporary file in a specified directory with given content.
     */
    static std::filesystem::path createTempFile(const std::filesystem::path &dir,
                                                const std::string &fileName,
                                                const std::string &content);

    /**
     * Parses command line arguments using Cli::CommandLineParser without process spawning.
     */
    static std::optional<Cli::CliOptions> parseArgs(const std::vector<std::string> &args,
                                                    std::string &errorMessage);

    /**
     * Directly runs the Cli::Driver in-process with the provided options.
     */
    static Cli::DriverResult runDriver(Cli::CliOptions options);

    /**
     * Executes the EzDslCli executable as a subprocess and captures stdout and exit code.
     */
    static int runCliProcess(const std::vector<std::string> &args, std::string &stdOut);

  private:
    class DiagnosticCollector *m_diagnosticCollector{ nullptr };
    class DiagnosticLogger *m_diagnosticLogger{ nullptr };
    class SourceManager *m_sourceManager{ nullptr };
    class SymbolTable *m_symbolTable{ nullptr };
    std::pmr::monotonic_buffer_resource m_allocator;
    size_t m_sourceCounter{ 0 };
};

/**
 * GoogleTest fixture wrapper that manages the lifecycle of EzDslCliTestSuite in SetUp() and TearDown(),
 * and provides a clean temporary filesystem sandbox directory for test artifacts.
 */
class EzDslCliTestSuiteAsGtest : public EzDslCliTestSuite, public ::testing::Test
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

#endif // EZDSLCLITESTSUITE_EZ_DSL_CLI_TEST_SUITE_H
