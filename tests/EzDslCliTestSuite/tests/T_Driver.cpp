#include "EzDslCliTestSuite.h"

#include <chrono>
#include <filesystem>

using namespace Cli;

class DriverTest : public EzDslCliTestSuiteAsGtest
{
};

// ============================================================================
// 1. File Handling & Error Cases
// ============================================================================

TEST_F(DriverTest, MissingInputFileReturnsError)
{
    CliOptions opts;
    opts.inputFilePath = (m_testTempDir / "non_existent.tyf").string();

    auto result = runDriver(opts);
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.errorMessage.find("does not exist") != std::string::npos);
}

TEST_F(DriverTest, UnknownExtensionWithoutGeneratorReturnsError)
{
    auto tempFile = createTempFile(m_testTempDir, "unknown.xyz", "integer i32(32);");

    CliOptions opts;
    opts.inputFilePath = tempFile.string();
    opts.outputPath = m_testTempDir.string();

    auto result = runDriver(opts);
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.errorMessage.find("Could not determine language dialect") != std::string::npos);
}

// ============================================================================
// 2. Dialect Detection & Explicit Generator Override
// ============================================================================

TEST_F(DriverTest, ExplicitGeneratorOverridesUnknownExtension)
{
    auto tempFile = createTempFile(m_testTempDir, "types.custom", "integer i32(32);");

    CliOptions opts;
    opts.inputFilePath = tempFile.string();
    opts.outputPath = m_testTempDir.string();
    opts.generator = GeneratorKind::TypeTable;

    auto result = runDriver(opts);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(std::filesystem::exists(m_testTempDir / "MirTypeTable.h"));
    EXPECT_TRUE(std::filesystem::exists(m_testTempDir / "MirTypeTable.cpp"));
}

TEST_F(DriverTest, ExplicitInstructionsGeneratorOverridesUnknownExtension)
{
    std::string irCode = R"(
        ir_inst Nop() {
            CATEGORY(System);
            TIER(HighLevel);
        }
    )";
    auto tempFile = createTempFile(m_testTempDir, "inst.custom", irCode);

    CliOptions opts;
    opts.inputFilePath = tempFile.string();
    opts.outputPath = m_testTempDir.string();
    opts.generator = GeneratorKind::Instructions;

    auto result = runDriver(opts);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(std::filesystem::exists(m_testTempDir / "MirInstructionSetDefs.h"));
}

// ============================================================================
// 3. Check-only and Dry-run Modes
// ============================================================================

TEST_F(DriverTest, CheckOnlyModeDoesNotProduceFiles)
{
    auto tempFile = createTempFile(m_testTempDir, "types.tyf", "integer i32(32); float f64(64);");

    CliOptions opts;
    opts.inputFilePath = tempFile.string();
    opts.outputPath = m_testTempDir.string();
    opts.checkOnly = true;

    auto result = runDriver(opts);
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(std::filesystem::exists(m_testTempDir / "MirTypeTable.h"));
    EXPECT_FALSE(std::filesystem::exists(m_testTempDir / "MirTypeTable.cpp"));
}

TEST_F(DriverTest, DryRunModeDoesNotProduceFiles)
{
    auto tempFile = createTempFile(m_testTempDir, "types.tyf", "integer i32(32); float f64(64);");

    CliOptions opts;
    opts.inputFilePath = tempFile.string();
    opts.outputPath = m_testTempDir.string();
    opts.dryRun = true;

    auto result = runDriver(opts);
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(std::filesystem::exists(m_testTempDir / "MirTypeTable.h"));
    EXPECT_FALSE(std::filesystem::exists(m_testTempDir / "MirTypeTable.cpp"));
}

// ============================================================================
// 4. Working Modes (Header-only and Source-only)
// ============================================================================

TEST_F(DriverTest, HeaderOnlyModeGeneratesOnlyHeader)
{
    auto tempFile = createTempFile(m_testTempDir, "types.tyf", "integer i16(16);");

    CliOptions opts;
    opts.inputFilePath = tempFile.string();
    opts.outputPath = m_testTempDir.string();
    opts.headerOnly = true;

    auto result = runDriver(opts);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(std::filesystem::exists(m_testTempDir / "MirTypeTable.h"));
    EXPECT_FALSE(std::filesystem::exists(m_testTempDir / "MirTypeTable.cpp"));
    EXPECT_EQ(result.generatedFiles.size(), 1);
    EXPECT_EQ(result.generatedFiles[0].role, "header");
}

TEST_F(DriverTest, SourceOnlyModeGeneratesOnlySource)
{
    auto tempFile = createTempFile(m_testTempDir, "types.tyf", "integer i16(16);");

    CliOptions opts;
    opts.inputFilePath = tempFile.string();
    opts.outputPath = m_testTempDir.string();
    opts.sourceOnly = true;

    auto result = runDriver(opts);
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(std::filesystem::exists(m_testTempDir / "MirTypeTable.h"));
    EXPECT_TRUE(std::filesystem::exists(m_testTempDir / "MirTypeTable.cpp"));
    EXPECT_EQ(result.generatedFiles.size(), 1);
    EXPECT_EQ(result.generatedFiles[0].role, "source");
}

// ============================================================================
// 5. Incremental Generation & Timestamp Preservation
// ============================================================================

TEST_F(DriverTest, TimestampPreservationOnUnchangedGeneration)
{
    auto tempFile = createTempFile(m_testTempDir, "types.tyf", "integer i32(32);");

    CliOptions opts;
    opts.inputFilePath = tempFile.string();
    opts.outputPath = m_testTempDir.string();

    // 1. Initial generation
    auto result1 = runDriver(opts);
    EXPECT_TRUE(result1.success);
    EXPECT_EQ(result1.generatedFiles.size(), 2);
    EXPECT_EQ(result1.unchangedFiles.size(), 0);

    auto hFile = m_testTempDir / "MirTypeTable.h";
    auto sFile = m_testTempDir / "MirTypeTable.cpp";

    auto pastTime = std::filesystem::last_write_time(hFile) - std::chrono::seconds(5);
    std::filesystem::last_write_time(hFile, pastTime);
    std::filesystem::last_write_time(sFile, pastTime);

    auto mtimeH1 = std::filesystem::last_write_time(hFile);
    auto mtimeS1 = std::filesystem::last_write_time(sFile);

    // 2. Re-run generation with unchanged source
    auto result2 = runDriver(opts);
    EXPECT_TRUE(result2.success);
    EXPECT_EQ(result2.generatedFiles.size(), 0);
    EXPECT_EQ(result2.unchangedFiles.size(), 2);

    auto mtimeH2 = std::filesystem::last_write_time(hFile);
    auto mtimeS2 = std::filesystem::last_write_time(sFile);

    EXPECT_EQ(mtimeH1, mtimeH2);
    EXPECT_EQ(mtimeS1, mtimeS2);
}

// ============================================================================
// 6. Error Diagnostics (Syntax and Semantic Failures)
// ============================================================================

TEST_F(DriverTest, SyntaxErrorFailsDriver)
{
    auto tempFile = createTempFile(m_testTempDir, "bad_syntax.tyf", "integer i32 ( unclosed ;");

    CliOptions opts;
    opts.inputFilePath = tempFile.string();
    opts.outputPath = m_testTempDir.string();
    opts.quiet = true;

    auto result = runDriver(opts);
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.errorMessage.find("Syntax parsing failed") != std::string::npos);
}

TEST_F(DriverTest, SemanticErrorFailsDriver)
{
    // Duplicate type definition fails semantic analysis
    auto tempFile = createTempFile(m_testTempDir, "dup_types.tyf", "integer i32(32); integer i32(32);");

    CliOptions opts;
    opts.inputFilePath = tempFile.string();
    opts.outputPath = m_testTempDir.string();
    opts.quiet = true;

    auto result = runDriver(opts);
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.errorMessage.find("Semantic analysis failed") != std::string::npos);
}

// ============================================================================
// 7. Dump Options Execution
// ============================================================================

TEST_F(DriverTest, DumpAllOptionsInTextAndJson)
{
    auto tempFile = createTempFile(m_testTempDir, "dump_test.tyf", "integer i32(32); float f64(64);");

    CliOptions opts;
    opts.inputFilePath = tempFile.string();
    opts.outputPath = m_testTempDir.string();
    opts.dumpInfo = true;
    opts.dumpAst = true;
    opts.dumpSymbols = true;
    opts.dumpFiles = true;
    opts.dryRun = true;

    // Text format
    opts.format = OutputFormat::Text;
    auto resultText = runDriver(opts);
    EXPECT_TRUE(resultText.success);

    // JSON format
    opts.format = OutputFormat::Json;
    auto resultJson = runDriver(opts);
    EXPECT_TRUE(resultJson.success);
}
