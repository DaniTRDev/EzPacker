#include "EzDslCliTestSuite.h"

#include <filesystem>
#include <fstream>

using namespace Cli;

class DriverLegalizeRulesTest : public EzDslCliTestSuiteAsGtest
{
  protected:
    std::string readFile(const std::filesystem::path &filePath)
    {
        std::ifstream file(filePath);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
};

// ============================================================================
// 1. Standalone Check-Only with Automatic Prelude
// ============================================================================

TEST_F(DriverLegalizeRulesTest, StandaloneLrdCheckOnlyWithPrelude)
{
    std::string lrdCode = R"dsl(
rule SDivPow2 {
    match {
        SDIV i32:$dst, i32:$lhs, imm(i32):$c;
    };
    when {
        isPowTwo($c);
        isPositiveConst($c);
    };
    emit {
        SAR i32:$dst, i32:$lhs, log2($c);
    };
};
)dsl";

    auto tempFile = createTempFile(m_testTempDir, "SDivRules.lrd", lrdCode);

    CliOptions opts;
    opts.inputFilePath = tempFile.string();
    opts.outputPath = m_testTempDir.string();
    opts.checkOnly = true;

    auto result = runDriver(opts);
    EXPECT_TRUE(result.success) << result.errorMessage;
    EXPECT_FALSE(std::filesystem::exists(m_testTempDir / "SDivRulesLegalizerRules.h"));
    EXPECT_FALSE(std::filesystem::exists(m_testTempDir / "SDivRulesLegalizerRules.cpp"));
}

// ============================================================================
// 2. Standalone Code Generation (--emit-rules)
// ============================================================================

TEST_F(DriverLegalizeRulesTest, StandaloneLrdEmitRulesGeneration)
{
    std::string lrdCode = R"dsl(
rule SDivPow2 {
    match {
        SDIV i32:$dst, i32:$lhs, imm(i32):$c;
    };
    when {
        isPowTwo($c);
    };
    emit {
        SAR i32:$dst, i32:$lhs, log2($c);
    };
};
)dsl";

    auto tempFile = createTempFile(m_testTempDir, "AMD64.lrd", lrdCode);

    CliOptions opts;
    opts.inputFilePath = tempFile.string();
    opts.outputPath = m_testTempDir.string();
    opts.targetName = "AMD64";
    opts.emitRules = true;

    auto result = runDriver(opts);
    EXPECT_TRUE(result.success) << result.errorMessage;

    auto hPath = m_testTempDir / "AMD64LegalizerRules.h";
    auto sPath = m_testTempDir / "AMD64LegalizerRules.cpp";

    EXPECT_TRUE(std::filesystem::exists(hPath));
    EXPECT_TRUE(std::filesystem::exists(sPath));

    std::string header = readFile(hPath);
    std::string source = readFile(sPath);

    EXPECT_NE(header.find("Rule_SDivPow2"), std::string::npos);
    EXPECT_NE(header.find("applyRules"), std::string::npos);
    EXPECT_NE(header.find("applyRuleById"), std::string::npos);

    EXPECT_NE(source.find("Rule_SDivPow2"), std::string::npos);
    EXPECT_NE(source.find("MirInstructionOpCode::SDIV"), std::string::npos);
    EXPECT_NE(source.find("MirInstructionOpCode::SAR"), std::string::npos);
    EXPECT_NE(source.find("isPowTwo"), std::string::npos);
}

// ============================================================================
// 3. Custom Dependencies (--types and --instructions)
// ============================================================================

TEST_F(DriverLegalizeRulesTest, CustomTypesAndInstructionsDependencies)
{
    std::string typesCode = "integer i32(32);";
    auto typesFile = createTempFile(m_testTempDir, "custom_types.tyf", typesCode);

    std::string instCode = R"dsl(
ir_inst SDIV(Register:dst OUT, Register:lhs IN, RegImm:rhs IN) {
    CATEGORY(Arithmetic);
    TIER(HighLevel);
    FLAGS(SizeMatch);
}

ir_inst SAR(Register:dst OUT, Register:val IN, RegIntImm:amt IN) {
    CATEGORY(Bitwise);
    TIER(HighLevel);
}
)dsl";
    auto instFile = createTempFile(m_testTempDir, "custom_inst.irdf", instCode);

    std::string lrdCode = R"dsl(
rule SDivPow2 {
    match {
        SDIV i32:$dst, i32:$lhs, imm(i32):$c;
    };
    when {
        isPowTwo($c);
    };
    emit {
        SAR i32:$dst, i32:$lhs, log2($c);
    };
};
)dsl";
    auto rulesFile = createTempFile(m_testTempDir, "CustomRules.lrd", lrdCode);

    CliOptions opts;
    opts.inputFilePath = rulesFile.string();
    opts.outputPath = m_testTempDir.string();
    opts.typesFilePath = typesFile.string();
    opts.instructionsFilePath = instFile.string();
    opts.emitRules = true;
    opts.targetName = "CustomTarget";

    auto result = runDriver(opts);
    EXPECT_TRUE(result.success) << result.errorMessage;

    EXPECT_TRUE(std::filesystem::exists(m_testTempDir / "CustomTargetLegalizerRules.h"));
    EXPECT_TRUE(std::filesystem::exists(m_testTempDir / "CustomTargetLegalizerRules.cpp"));
}

// ============================================================================
// 4. Paired Discovery (.lad + .lrd with CUSTOM >> Rule)
// ============================================================================

TEST_F(DriverLegalizeRulesTest, PairedDiscoveryWithCustomRule)
{
    std::string lrdCode = R"dsl(
rule SDivPow2 {
    match {
        SDIV i32:$dst, i32:$lhs, imm(i32):$c;
    };
    when {
        isPowTwo($c);
    };
    emit {
        SAR i32:$dst, i32:$lhs, log2($c);
    };
};
)dsl";
    // Create companion .lrd adjacent to .lad
    createTempFile(m_testTempDir, "AMD64.lrd", lrdCode);

    std::string ladCode = R"dsl(
target AMD64;

action SDIV {
    CUSTOM >> SDivPow2;
};
)dsl";
    auto ladFile = createTempFile(m_testTempDir, "AMD64.lad", ladCode);

    CliOptions opts;
    opts.inputFilePath = ladFile.string();
    opts.outputPath = m_testTempDir.string();
    opts.targetName = "AMD64";
    opts.generator = GeneratorKind::Legalizer;

    auto result = runDriver(opts);
    EXPECT_TRUE(result.success) << result.errorMessage;

    // Both action table and rules table must be produced
    auto actionHeader = m_testTempDir / "AMD64LegalizerActionTable.h";
    auto actionSource = m_testTempDir / "AMD64LegalizerActionTable.cpp";
    auto rulesHeader = m_testTempDir / "AMD64LegalizerRules.h";
    auto rulesSource = m_testTempDir / "AMD64LegalizerRules.cpp";

    EXPECT_TRUE(std::filesystem::exists(actionHeader));
    EXPECT_TRUE(std::filesystem::exists(actionSource));
    EXPECT_TRUE(std::filesystem::exists(rulesHeader));
    EXPECT_TRUE(std::filesystem::exists(rulesSource));

    std::string actionSrcContent = readFile(actionSource);
    EXPECT_NE(actionSrcContent.find("#include \"AMD64LegalizerRules.h\""), std::string::npos);
    EXPECT_NE(actionSrcContent.find("applyRuleById"), std::string::npos);
}

// ============================================================================
// 5. Dump Operations (--dump-ast and --dump-symbols)
// ============================================================================

TEST_F(DriverLegalizeRulesTest, DumpAstAndSymbolsOnLegalizeRule)
{
    std::string lrdCode = R"dsl(
rule SDivPow2 {
    match {
        SDIV i32:$dst, i32:$lhs, imm(i32):$c;
    };
    when {
        isPowTwo($c);
    };
    emit {
        SAR i32:$dst, i32:$lhs, log2($c);
    };
};
)dsl";
    auto rulesFile = createTempFile(m_testTempDir, "DumpRules.lrd", lrdCode);

    CliOptions opts;
    opts.inputFilePath = rulesFile.string();
    opts.outputPath = m_testTempDir.string();
    opts.checkOnly = true;
    opts.dumpAst = true;
    opts.dumpSymbols = true;
    opts.dumpInfo = true;

    auto result = runDriver(opts);
    EXPECT_TRUE(result.success) << result.errorMessage;
}
