#include "EzDslCliTestSuite.h"

using namespace Cli;

class CommandLineParserTest : public EzDslCliTestSuiteAsGtest
{
};

// ============================================================================
// 1. Basic Options & Input/Output Validation
// ============================================================================

TEST_F(CommandLineParserTest, MissingInputFails)
{
    std::string err;
    auto opts = parseArgs({}, err);
    EXPECT_FALSE(opts.has_value());
    EXPECT_TRUE(err.find("Missing required input file") != std::string::npos);

    // Only output specified
    err.clear();
    opts = parseArgs({ "-o", "out_dir" }, err);
    EXPECT_FALSE(opts.has_value());
    EXPECT_TRUE(err.find("Missing required input file") != std::string::npos);
}

TEST_F(CommandLineParserTest, BasicInputAndOutputShortFlags)
{
    std::string err;
    auto opts = parseArgs({ "-i", "types.tyf", "-o", "output/gen" }, err);
    ASSERT_TRUE(opts.has_value());
    EXPECT_EQ(opts->inputFilePath, "types.tyf");
    EXPECT_EQ(opts->outputPath, "output/gen");
    EXPECT_EQ(opts->generator, GeneratorKind::Auto);
    EXPECT_EQ(opts->dialect, LanguageDialect::Auto);
    EXPECT_FALSE(opts->headerOnly);
    EXPECT_FALSE(opts->sourceOnly);
    EXPECT_FALSE(opts->dryRun);
    EXPECT_FALSE(opts->checkOnly);
    EXPECT_FALSE(opts->verbose);
    EXPECT_FALSE(opts->quiet);
    EXPECT_EQ(opts->format, OutputFormat::Text);
}

TEST_F(CommandLineParserTest, BasicInputAndOutputLongFlags)
{
    std::string err;
    auto opts = parseArgs({ "--input", "instructions.irdf", "--output", "custom/bin" }, err);
    ASSERT_TRUE(opts.has_value());
    EXPECT_EQ(opts->inputFilePath, "instructions.irdf");
    EXPECT_EQ(opts->outputPath, "custom/bin");
}

TEST_F(CommandLineParserTest, DefaultOutputPathIsCurrentDirectory)
{
    std::string err;
    auto opts = parseArgs({ "-i", "types.tyf" }, err);
    ASSERT_TRUE(opts.has_value());
    EXPECT_EQ(opts->outputPath, ".");
}

// ============================================================================
// 2. Include Directories
// ============================================================================

TEST_F(CommandLineParserTest, MultipleIncludeDirectories)
{
    std::string err;
    auto opts = parseArgs({ "-i", "types.tyf", "-I", "include/core", "-I", "include/mir", "-I", "vendor/dsl" }, err);
    ASSERT_TRUE(opts.has_value());
    ASSERT_EQ(opts->includeDirs.size(), 3);
    EXPECT_EQ(opts->includeDirs[0], "include/core");
    EXPECT_EQ(opts->includeDirs[1], "include/mir");
    EXPECT_EQ(opts->includeDirs[2], "vendor/dsl");
}

// ============================================================================
// 3. Generator Selection & Conflict Resolution
// ============================================================================

TEST_F(CommandLineParserTest, EmitTypeTableFlag)
{
    std::string err;
    auto opts = parseArgs({ "-i", "types.tyf", "--emit-type-table" }, err);
    ASSERT_TRUE(opts.has_value());
    EXPECT_EQ(opts->generator, GeneratorKind::TypeTable);
}

TEST_F(CommandLineParserTest, EmitInstructionsFlag)
{
    std::string err;
    auto opts = parseArgs({ "-i", "instructions.irdf", "--emit-instructions" }, err);
    ASSERT_TRUE(opts.has_value());
    EXPECT_EQ(opts->generator, GeneratorKind::Instructions);
}

TEST_F(CommandLineParserTest, ExplicitGeneratorFlagTypeTable)
{
    std::string err;
    auto opts1 = parseArgs({ "-i", "types.tyf", "--generator", "type-table" }, err);
    ASSERT_TRUE(opts1.has_value());
    EXPECT_EQ(opts1->generator, GeneratorKind::TypeTable);

    auto opts2 = parseArgs({ "-i", "types.tyf", "--generator", "typetable" }, err);
    ASSERT_TRUE(opts2.has_value());
    EXPECT_EQ(opts2->generator, GeneratorKind::TypeTable);

    auto opts3 = parseArgs({ "-i", "types.tyf", "--generator", "TYPE-TABLE" }, err);
    ASSERT_TRUE(opts3.has_value());
    EXPECT_EQ(opts3->generator, GeneratorKind::TypeTable);
}

TEST_F(CommandLineParserTest, ExplicitGeneratorFlagInstructions)
{
    std::string err;
    auto opts1 = parseArgs({ "-i", "instructions.irdf", "--generator", "instructions" }, err);
    ASSERT_TRUE(opts1.has_value());
    EXPECT_EQ(opts1->generator, GeneratorKind::Instructions);

    auto opts2 = parseArgs({ "-i", "instructions.irdf", "--generator", "instruction" }, err);
    ASSERT_TRUE(opts2.has_value());
    EXPECT_EQ(opts2->generator, GeneratorKind::Instructions);
}

TEST_F(CommandLineParserTest, ExplicitGeneratorFlagAuto)
{
    std::string err;
    auto opts = parseArgs({ "-i", "any.dsl", "--generator", "auto" }, err);
    ASSERT_TRUE(opts.has_value());
    EXPECT_EQ(opts->generator, GeneratorKind::Auto);
}

TEST_F(CommandLineParserTest, ConflictingGeneratorsFail)
{
    std::string err;
    auto opts = parseArgs({ "-i", "types.tyf", "--emit-type-table", "--emit-instructions" }, err);
    EXPECT_FALSE(opts.has_value());
    EXPECT_TRUE(err.find("Cannot specify both --emit-type-table and --emit-instructions simultaneously.") !=
                std::string::npos);
}

// ============================================================================
// 4. Working Modes & Verification Flags
// ============================================================================

TEST_F(CommandLineParserTest, HeaderOnlyAndSourceOnlyFlags)
{
    std::string err;
    auto optsH = parseArgs({ "-i", "types.tyf", "--header-only" }, err);
    ASSERT_TRUE(optsH.has_value());
    EXPECT_TRUE(optsH->headerOnly);
    EXPECT_FALSE(optsH->sourceOnly);

    auto optsS = parseArgs({ "-i", "types.tyf", "--source-only" }, err);
    ASSERT_TRUE(optsS.has_value());
    EXPECT_FALSE(optsS->headerOnly);
    EXPECT_TRUE(optsS->sourceOnly);
}

TEST_F(CommandLineParserTest, CheckOnlyAndDryRunFlags)
{
    std::string err;
    auto optsCheck = parseArgs({ "-i", "types.tyf", "--check-only" }, err);
    ASSERT_TRUE(optsCheck.has_value());
    EXPECT_TRUE(optsCheck->checkOnly);

    auto optsDry = parseArgs({ "-i", "types.tyf", "--dry-run" }, err);
    ASSERT_TRUE(optsDry.has_value());
    EXPECT_TRUE(optsDry->dryRun);
}

TEST_F(CommandLineParserTest, VerboseAndQuietFlags)
{
    std::string err;
    auto optsV1 = parseArgs({ "-i", "types.tyf", "-v" }, err);
    ASSERT_TRUE(optsV1.has_value());
    EXPECT_TRUE(optsV1->verbose);

    auto optsV2 = parseArgs({ "-i", "types.tyf", "--verbose" }, err);
    ASSERT_TRUE(optsV2.has_value());
    EXPECT_TRUE(optsV2->verbose);

    auto optsQ1 = parseArgs({ "-i", "types.tyf", "-q" }, err);
    ASSERT_TRUE(optsQ1.has_value());
    EXPECT_TRUE(optsQ1->quiet);

    auto optsQ2 = parseArgs({ "-i", "types.tyf", "--quiet" }, err);
    ASSERT_TRUE(optsQ2.has_value());
    EXPECT_TRUE(optsQ2->quiet);
}

// ============================================================================
// 5. Dump and Format Flags
// ============================================================================

TEST_F(CommandLineParserTest, DumpFlags)
{
    std::string err;
    auto opts = parseArgs({ "-i", "types.tyf", "--dump-info", "--dump-ast", "--dump-symbols", "--dump-files" }, err);
    ASSERT_TRUE(opts.has_value());
    EXPECT_TRUE(opts->dumpInfo);
    EXPECT_TRUE(opts->dumpAst);
    EXPECT_TRUE(opts->dumpSymbols);
    EXPECT_TRUE(opts->dumpFiles);
}

TEST_F(CommandLineParserTest, FormatFlagTextAndJson)
{
    std::string err;
    auto optsJson1 = parseArgs({ "-i", "types.tyf", "--format", "json" }, err);
    ASSERT_TRUE(optsJson1.has_value());
    EXPECT_EQ(optsJson1->format, OutputFormat::Json);

    auto optsJson2 = parseArgs({ "-i", "types.tyf", "--format", "JSON" }, err);
    ASSERT_TRUE(optsJson2.has_value());
    EXPECT_EQ(optsJson2->format, OutputFormat::Json);

    auto optsText = parseArgs({ "-i", "types.tyf", "--format", "text" }, err);
    ASSERT_TRUE(optsText.has_value());
    EXPECT_EQ(optsText->format, OutputFormat::Text);
}

// ============================================================================
// 6. Version, Help & Unknown Arguments
// ============================================================================

TEST_F(CommandLineParserTest, VersionFlagReturnsNulloptWithoutError)
{
    std::string err;
    auto opts = parseArgs({ "--version" }, err);
    EXPECT_FALSE(opts.has_value());
    EXPECT_TRUE(err.empty());
}

TEST_F(CommandLineParserTest, VersionAndHelpStrings)
{
    CommandLineParser parser;
    std::string ver = parser.getVersion();
    EXPECT_TRUE(ver.find("EzDslCli version 1.0.0") != std::string::npos);

    std::string help = parser.getHelp();
    EXPECT_TRUE(help.find("EzDSL Compiler Backend Driver & Code Generator Tool") != std::string::npos);
    EXPECT_TRUE(help.find("--emit-type-table") != std::string::npos);
    EXPECT_TRUE(help.find("--emit-instructions") != std::string::npos);
}

TEST_F(CommandLineParserTest, UnknownArgumentFails)
{
    std::string err;
    auto opts = parseArgs({ "-i", "types.tyf", "--completely-unknown-flag" }, err);
    EXPECT_FALSE(opts.has_value());
    EXPECT_FALSE(err.empty());
}
