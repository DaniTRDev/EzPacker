#include "EzCompilerTestSuite.h"

using namespace EzCompiler;

// Parses a bare input file and applies the default object stage, O0 optimization, and off flags.
TEST_F(EzCompilerTestSuite, TestDefaultCommandLineOptions)
{
    CommandLineParser parser;
    CommandLineOptions options;
    std::string err;

    std::vector<std::string> args = { "ezc", "main.ez" };
    bool ok = parser.parse(args, options, err);

    EXPECT_TRUE(ok);
    EXPECT_EQ(options.inputFilePath, "main.ez");
    EXPECT_EQ(options.emissionStage, EmissionStage::Object);
    EXPECT_EQ(options.optLevel, OptimizationLevel::O0);
    EXPECT_FALSE(options.verbose);
    EXPECT_FALSE(options.isPositionIndependent);
}

// Parses -o and --target, resolving the target triple into its arch, system, and ABI parts.
TEST_F(EzCompilerTestSuite, TestCustomOutputAndTarget)
{
    CommandLineParser parser;
    CommandLineOptions options;
    std::string err;

    std::vector<std::string> args = { "ezc", "test.ez", "-o", "myprog.o", "--target", "x86_64-linux-gnu" };
    bool ok = parser.parse(args, options, err);

    EXPECT_TRUE(ok);
    EXPECT_EQ(options.inputFilePath, "test.ez");
    EXPECT_EQ(options.outputFilePath, "myprog.o");
    EXPECT_EQ(options.target.getArch(), "x86_64");
    EXPECT_EQ(options.target.getSys(), "linux");
    EXPECT_EQ(options.target.getAbi(), "gnu");
    EXPECT_TRUE(options.target.isLinux());
    EXPECT_TRUE(options.target.isElf());
}

// Verifies each emission-stage flag maps to the expected pipeline stage, including -S for assembly.
TEST_F(EzCompilerTestSuite, TestEmissionStageFlags)
{
    CommandLineParser parser;
    CommandLineOptions options;
    std::string err;

    // --emit-mir
    {
        std::vector<std::string> args = { "ezc", "test.ez", "--emit-mir" };
        EXPECT_TRUE(parser.parse(args, options, err));
        EXPECT_EQ(options.emissionStage, EmissionStage::GenericMir);
    }

    // --emit-legalized-mir
    {
        std::vector<std::string> args = { "ezc", "test.ez", "--emit-legalized-mir" };
        EXPECT_TRUE(parser.parse(args, options, err));
        EXPECT_EQ(options.emissionStage, EmissionStage::LegalizedMir);
    }

    // --emit-lowered-mir
    {
        std::vector<std::string> args = { "ezc", "test.ez", "--emit-lowered-mir" };
        EXPECT_TRUE(parser.parse(args, options, err));
        EXPECT_EQ(options.emissionStage, EmissionStage::LoweredMir);
    }

    // -S (Assembly)
    {
        std::vector<std::string> args = { "ezc", "test.ez", "-S" };
        EXPECT_TRUE(parser.parse(args, options, err));
        EXPECT_EQ(options.emissionStage, EmissionStage::Assembly);
    }
}

// Parses combined optimization, verbosity, pass-reporting, and position-independent flags.
TEST_F(EzCompilerTestSuite, TestOptimizationAndVerboseFlags)
{
    CommandLineParser parser;
    CommandLineOptions options;
    std::string err;

    std::vector<std::string> args = { "ezc", "test.ez", "-O2", "-v", "--print-passes", "--time-passes", "-fPIC" };
    bool ok = parser.parse(args, options, err);

    EXPECT_TRUE(ok);
    EXPECT_EQ(options.optLevel, OptimizationLevel::O2);
    EXPECT_TRUE(options.verbose);
    EXPECT_TRUE(options.printPasses);
    EXPECT_TRUE(options.timePasses);
    EXPECT_TRUE(options.isPositionIndependent);
}

// Rejects an unrecognized flag and reports a non-empty error message.
TEST_F(EzCompilerTestSuite, TestInvalidFlagHandling)
{
    CommandLineParser parser;
    CommandLineOptions options;
    std::string err;

    std::vector<std::string> args = { "ezc", "--unknown-flag-that-does-not-exist" };
    bool ok = parser.parse(args, options, err);

    EXPECT_FALSE(ok);
    EXPECT_FALSE(err.empty());
}
