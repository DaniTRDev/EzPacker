#include "EzCompilerTestSuite.h"
#include "Rtti/RttiDescriptor.h"

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

// WEI-09: contradictory stage flags must be diagnosed instead of silently picking one.
TEST_F(EzCompilerTestSuite, TestConflictingEmissionFlagsRejected)
{
    CommandLineParser parser;
    CommandLineOptions options;
    std::string err;

    std::vector<std::string> args = { "ezc", "test.ez", "--emit-mir", "-S" };
    EXPECT_FALSE(parser.parse(args, options, err));
    EXPECT_FALSE(err.empty());
}

// WEI-09: contradictory optimization flags must be diagnosed.
TEST_F(EzCompilerTestSuite, TestConflictingOptimizationFlagsRejected)
{
    CommandLineParser parser;
    CommandLineOptions options;
    std::string err;

    std::vector<std::string> args = { "ezc", "test.ez", "-O2", "-Os" };
    EXPECT_FALSE(parser.parse(args, options, err));
    EXPECT_FALSE(err.empty());
}

// WEI-09: an unknown --diag-level value is a hard error rather than a silent default.
TEST_F(EzCompilerTestSuite, TestUnknownDiagLevelRejected)
{
    CommandLineParser parser;
    CommandLineOptions options;
    std::string err;

    std::vector<std::string> args = { "ezc", "test.ez", "--diag-level", "bogus" };
    EXPECT_FALSE(parser.parse(args, options, err));
    EXPECT_FALSE(err.empty());

    // A documented level still parses.
    std::vector<std::string> valid = { "ezc", "test.ez", "--diag-level", "trace" };
    EXPECT_TRUE(parser.parse(valid, options, err));
    EXPECT_EQ(options.diagThreshold, DiagnosticMessageType::Diag_Trace);
}

// Target feature flags: --target-feature, -mattr, and -m<feature> / -mno-<feature>
TEST_F(EzCompilerTestSuite, TestTargetFeatureCommandLineOptions)
{
    CommandLineParser parser;
    CommandLineOptions options;
    std::string err;

    // 1. --target-feature
    {
        std::vector<std::string> args = { "ezc", "main.ez", "--target-feature", "+avx", "--target-feature", "-sse" };
        EXPECT_TRUE(parser.parse(args, options, err)) << "Parse error: " << err;
        ASSERT_EQ(options.targetFeatures.size(), 2u);
        EXPECT_EQ(options.targetFeatures[0], "+avx");
        EXPECT_EQ(options.targetFeatures[1], "-sse");
    }

    // 2. -mattr
    {
        std::vector<std::string> args = { "ezc", "main.ez", "-mattr=+avx2,-sse4.1" };
        EXPECT_TRUE(parser.parse(args, options, err));
        ASSERT_EQ(options.targetFeatures.size(), 2u);
        EXPECT_EQ(options.targetFeatures[0], "+avx2");
        EXPECT_EQ(options.targetFeatures[1], "-sse4.1");
    }

    // 3. Dynamic machine flags: -mavx, -mno-avx, -msse2
    {
        std::vector<std::string> args = { "ezc", "main.ez", "-mavx", "-mno-sse" };
        EXPECT_TRUE(parser.parse(args, options, err));
        ASSERT_EQ(options.targetFeatures.size(), 2u);
        EXPECT_EQ(options.targetFeatures[0], "+avx");
        EXPECT_EQ(options.targetFeatures[1], "-sse");
    }

    // 4. Combined machine flags, --target-feature, and -mattr
    {
        std::vector<std::string> args = { "ezc", "main.ez", "-mavx", "--target-feature", "+sse4.2", "-mattr=+bmi" };
        EXPECT_TRUE(parser.parse(args, options, err));
        ASSERT_EQ(options.targetFeatures.size(), 3u);
        EXPECT_EQ(options.targetFeatures[0], "+avx");
        EXPECT_EQ(options.targetFeatures[1], "+sse4.2");
        EXPECT_EQ(options.targetFeatures[2], "+bmi");
    }
}

// RTTI flags: default enabled, --no-rtti, -fno-rtti, --rtti, and conflicting flag rejection
TEST_F(EzCompilerTestSuite, TestRttiCommandLineOptions)
{
    CommandLineParser parser;
    CommandLineOptions options;
    std::string err;

    // 1. Default (no RTTI flags passed) -> enabled
    {
        std::vector<std::string> args = { "ezc", "main.ez" };
        EXPECT_TRUE(parser.parse(args, options, err));
        EXPECT_TRUE(options.enableRtti);
    }

    // 2. Explicit --no-rtti -> disabled
    {
        std::vector<std::string> args = { "ezc", "main.ez", "--no-rtti" };
        EXPECT_TRUE(parser.parse(args, options, err));
        EXPECT_FALSE(options.enableRtti);
    }

    // 3. -fno-rtti alias -> disabled
    {
        std::vector<std::string> args = { "ezc", "main.ez", "-fno-rtti" };
        EXPECT_TRUE(parser.parse(args, options, err));
        EXPECT_FALSE(options.enableRtti);
    }

    // 4. Explicit --rtti -> enabled
    {
        std::vector<std::string> args = { "ezc", "main.ez", "--rtti" };
        EXPECT_TRUE(parser.parse(args, options, err));
        EXPECT_TRUE(options.enableRtti);
    }

    // 5. Conflicting flags -> error
    {
        std::vector<std::string> args = { "ezc", "main.ez", "--rtti", "--no-rtti" };
        EXPECT_FALSE(parser.parse(args, options, err));
        EXPECT_NE(err.find("conflicting RTTI flags"), std::string::npos);
    }
}

// Verifies EzCore::RttiTypeDescriptor layout, hashing, and isA() subtyping checks
TEST_F(EzCompilerTestSuite, TestRttiDescriptorHierarchy)
{
    using namespace EzCore;

    // Base exception: Exception
    RttiTypeDescriptor baseDesc{
        .typeId = computeTypeId("Exception"),
        .typeName = "Exception",
        .numBases = 0,
        .bases = nullptr,
        .declarationSite = {
            .filePath = "std/exception.ez",
            .functionName = "",
            .line = 10,
            .column = 1,
            .snippet = "class Exception;"
        }
    };

    const RttiTypeDescriptor *ioBaseArray[] = { &baseDesc };

    // Derived exception: IOException extends Exception
    RttiTypeDescriptor ioDesc{
        .typeId = computeTypeId("IOException"),
        .typeName = "IOException",
        .numBases = 1,
        .bases = ioBaseArray,
        .declarationSite = {
            .filePath = "std/io.ez",
            .functionName = "",
            .line = 42,
            .column = 5,
            .snippet = "class IOException : Exception;"
        }
    };

    const RttiTypeDescriptor *fnfBaseArray[] = { &ioDesc };

    // Sub-derived: FileNotFoundException extends IOException
    RttiTypeDescriptor fnfDesc{
        .typeId = computeTypeId("FileNotFoundException"),
        .typeName = "FileNotFoundException",
        .numBases = 1,
        .bases = fnfBaseArray,
        .declarationSite = {
            .filePath = "std/fs.ez",
            .functionName = "",
            .line = 105,
            .column = 5,
            .snippet = "class FileNotFoundException : IOException;"
        }
    };

    // Subtyping checks
    EXPECT_TRUE(baseDesc.isA(&baseDesc));
    EXPECT_TRUE(ioDesc.isA(&baseDesc));
    EXPECT_TRUE(fnfDesc.isA(&baseDesc));
    EXPECT_TRUE(fnfDesc.isA(&ioDesc));
    EXPECT_FALSE(baseDesc.isA(&ioDesc));
    EXPECT_FALSE(ioDesc.isA(&fnfDesc));

    // Throw site payload
    RichExceptionPayload payload{
        .payload = nullptr,
        .rtti = &fnfDesc,
        .throwSite = {
            .filePath = "main.ez",
            .functionName = "openFile",
            .line = 77,
            .column = 12,
            .snippet = "throw new FileNotFoundException(\"data.bin\");"
        }
    };

    EXPECT_EQ(payload.rtti->typeName, "FileNotFoundException");
    EXPECT_TRUE(payload.rtti->isA(&baseDesc));
    EXPECT_EQ(payload.throwSite.functionName, "openFile");
    EXPECT_EQ(payload.throwSite.line, 77);
}

TEST_F(EzCompilerTestSuite, TestDefaultExceptionConstants)
{
    using namespace EzCore;
    EXPECT_EQ(kDefaultExceptionTypeName, "EzDefaultException");
    EXPECT_EQ(kDefaultExceptionTypeId, computeTypeId("EzDefaultException"));
    EXPECT_NE(kDefaultExceptionTypeId, 0u);
}


