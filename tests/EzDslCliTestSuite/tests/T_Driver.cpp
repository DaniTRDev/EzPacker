#include "EzDslCliTestSuite.h"

#include <chrono>
#include <filesystem>

using namespace Cli;

/**
 * Fixture for exercising the EzDslCli driver end to end with temporary source files.
 */
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

// Verifies an unknown file extension with no explicit generator fails dialect detection.
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

// Verifies an explicit instructions generator works despite an unknown extension.
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

// Verifies --dry-run runs the pipeline but writes no output files.
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

// Verifies source-only mode emits just the source and reports a single source artifact.
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

// Verifies a semantic error (duplicate type) aborts the driver with a semantic failure message.
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

// ============================================================================
// 8. Calling Convention Driver Execution
// ============================================================================

TEST_F(DriverTest, CallingConvDriverExecution)
{
    std::string ccSource = R"(
    calling_convention Win64 {
        stack {
            align: 16,
            growth: down,
            cleanup: caller,
            shadow_space: 32,
            sp: rsp,
            fp: rbp
        }

        preserve callee: [rbx, rbp, rdi, rsi, r12, r13, r14, r15]
        preserve caller: [rax, rcx, rdx, r8, r9, r10, r11]

        classify {
            types [i8, i16, i32, i64, ptr] => integer
            types [f32, f64] => sse
            aggregate {
                when non_trivial || unaligned || size > 64 => memory,
                slice: 8 bytes,
                precedence: [memory, integer, sse],
                policy: all_or_nothing
            }
        }

        arguments {
            pass integer => seq([rcx, rdx, r8, r9]), fallback: stack(8)
            pass sse => seq([xmm0, xmm1, xmm2, xmm3]), fallback: stack(8)
            pass memory => stack(8)
        }

        returns {
            sret {
                ptr: rcx,
                consumes_slot: true,
                returns: rax
            }
            pass integer => seq([rax])
            pass sse => seq([xmm0])
        }
    }
    )";

    auto tempFile = createTempFile(m_testTempDir, "win64.ezcc", ccSource);

    CliOptions opts;
    opts.inputFilePath = tempFile.string();
    opts.outputPath = m_testTempDir.string();
    opts.targetName = "Win64";

    auto result = runDriver(opts);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(std::filesystem::exists(m_testTempDir / "Win64CallingConvDesc.h"));
    EXPECT_TRUE(std::filesystem::exists(m_testTempDir / "Win64CallingConvDesc.cpp"));
}

// Verifies an invalid stack alignment aborts calling-convention generation.
TEST_F(DriverTest, CallingConvDriverSemanticErrorFails)
{
    std::string ccBadSource = R"(
    calling_convention BadCC {
        stack {
            align: 15,
            growth: down,
            cleanup: caller,
            shadow_space: 0,
            sp: rsp,
            fp: rbp
        }
    }
    )";

    auto tempFile = createTempFile(m_testTempDir, "bad_cc.ezcc", ccBadSource);

    CliOptions opts;
    opts.inputFilePath = tempFile.string();
    opts.outputPath = m_testTempDir.string();
    opts.quiet = true;

    auto result = runDriver(opts);
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.errorMessage.find("Semantic analysis failed") != std::string::npos);
}
