#include "EzCompilerTestSuite.h"
#include "FrontendAdapter.h"
#include <fstream>
#include <filesystem>
#include <string>

using namespace EzCompiler;

// Verifies that CompilationPipeline immediately rejects input MIR with SizeMatch violations.
TEST_F(EzCompilerTestSuite, TestPipelineRejectsSizeMismatch)
{
    const std::string mirPath = "test_bad_size.mir";
    if (std::filesystem::exists(mirPath))
    {
        std::filesystem::remove(mirPath);
    }

    std::string_view mirContent = R"mir(
fn @bad_size(i32 %a, i64 %b) -> i32 {
entry:
    %res = ADD i32 %a, %b;
    RET i32 %res;
}
)mir";

    {
        std::ofstream out(mirPath);
        out << mirContent;
    }

    CommandLineOptions options;
    options.inputFilePath = mirPath;
    options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");
    options.optLevel = OptimizationLevel::O0;
    options.emissionStage = EmissionStage::GenericMir;

    DriverContext ctx(options);
    ASSERT_TRUE(ctx.initialize());
    ASSERT_TRUE(MirModuleLoader::loadMirFile(ctx, mirPath, *ctx.getBuilderContext()));

    CompilationPipeline pipeline(ctx);
    // Verification pass should fail before middle-end begins
    EXPECT_FALSE(pipeline.runPipeline());

    std::filesystem::remove(mirPath);
}

// Verifies that CompilationPipeline immediately rejects input MIR with DestLarger violations.
TEST_F(EzCompilerTestSuite, TestPipelineRejectsDestLargerViolation)
{
    const std::string mirPath = "test_bad_zext.mir";
    if (std::filesystem::exists(mirPath))
    {
        std::filesystem::remove(mirPath);
    }

    std::string_view mirContent = R"mir(
fn @bad_zext(i32 %a) -> i16 {
entry:
    %res = ZEXT i16 %a;
    RET i16 %res;
}
)mir";

    {
        std::ofstream out(mirPath);
        out << mirContent;
    }

    CommandLineOptions options;
    options.inputFilePath = mirPath;
    options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");
    options.optLevel = OptimizationLevel::O0;
    options.emissionStage = EmissionStage::GenericMir;

    DriverContext ctx(options);
    ASSERT_TRUE(ctx.initialize());
    ASSERT_TRUE(MirModuleLoader::loadMirFile(ctx, mirPath, *ctx.getBuilderContext()));

    CompilationPipeline pipeline(ctx);
    EXPECT_FALSE(pipeline.runPipeline());

    std::filesystem::remove(mirPath);
}

// Verifies that CompilationPipeline successfully processes valid input MIR with the verifier enabled.
TEST_F(EzCompilerTestSuite, TestPipelineAcceptsValidMir)
{
    const std::string mirPath = "test_valid_pipeline.mir";
    if (std::filesystem::exists(mirPath))
    {
        std::filesystem::remove(mirPath);
    }

    std::string_view mirContent = R"mir(
fn @valid_add(i64 %a, i64 %b) -> i64 {
entry:
    %res = ADD i64 %a, %b;
    RET i64 %res;
}
)mir";

    {
        std::ofstream out(mirPath);
        out << mirContent;
    }

    CommandLineOptions options;
    options.inputFilePath = mirPath;
    options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");
    options.optLevel = OptimizationLevel::O0;
    options.emissionStage = EmissionStage::GenericMir;

    DriverContext ctx(options);
    ASSERT_TRUE(ctx.initialize());
    ASSERT_TRUE(MirModuleLoader::loadMirFile(ctx, mirPath, *ctx.getBuilderContext()));

    CompilationPipeline pipeline(ctx);
    EXPECT_TRUE(pipeline.runPipeline());

    std::filesystem::remove(mirPath);
}
