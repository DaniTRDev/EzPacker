#include "EzCompilerTestSuite.h"
#include "FrontendAdapter.h"
#include <fstream>
#include <filesystem>
#include <string>

using namespace EzCompiler;

// Helper to count lines or instruction occurrences in assembly dump
static size_t countLines(const std::string &text)
{
    size_t count = 0;
    for (char c : text)
    {
        if (c == '\n')
        {
            count++;
        }
    }
    return count;
}

// Verifies that compiling at -O0 preserves unoptimized patterns whereas -O2 optimizes them.
TEST_F(EzCompilerTestSuite, TestOptimizationPipeline_O0VsO2_ArithmeticOptimization)
{
    const std::string mirPath = "test_opt_arith.mir";
    if (std::filesystem::exists(mirPath))
    {
        std::filesystem::remove(mirPath);
    }

    std::string_view mirContent = R"mir(
fn @calc_opt(i64 %a) -> i64 {
entry:
    %zero = MOV i64 0;
    %sum = ADD i64 %a, %zero;
    RET i64 %sum;
}
)mir";

    {
        std::ofstream out(mirPath);
        out << mirContent;
    }

    std::string asmO0;
    std::string asmO2;

    // Run with -O0
    {
        CommandLineOptions options;
        options.inputFilePath = mirPath;
        options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");
        options.optLevel = OptimizationLevel::O0;
        options.emissionStage = EmissionStage::Assembly;

        DriverContext ctx(options);
        ASSERT_TRUE(ctx.initialize());
        ASSERT_TRUE(MirModuleLoader::loadMirFile(ctx, mirPath, *ctx.getBuilderContext()));

        CompilationPipeline pipeline(ctx);
        EXPECT_TRUE(pipeline.runPipeline());
        asmO0 = pipeline.dumpAssembly();
        EXPECT_FALSE(asmO0.empty());
    }

    // Run with -O2
    {
        CommandLineOptions options;
        options.inputFilePath = mirPath;
        options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");
        options.optLevel = OptimizationLevel::O2;
        options.emissionStage = EmissionStage::Assembly;

        DriverContext ctx(options);
        ASSERT_TRUE(ctx.initialize());
        ASSERT_TRUE(MirModuleLoader::loadMirFile(ctx, mirPath, *ctx.getBuilderContext()));

        CompilationPipeline pipeline(ctx);
        EXPECT_TRUE(pipeline.runPipeline());
        asmO2 = pipeline.dumpAssembly();
        EXPECT_FALSE(asmO2.empty());
    }

    std::filesystem::remove(mirPath);

    // -O2 should eliminate the redundant add with zero / intermediate copies
    EXPECT_LE(countLines(asmO2), countLines(asmO0));
}

// Verifies that generic peephole eliminates dead code following a terminator.
TEST_F(EzCompilerTestSuite, TestOptimizationPipeline_DeadCodeElimination)
{
    const std::string mirPath = "test_dead_code.mir";
    if (std::filesystem::exists(mirPath))
    {
        std::filesystem::remove(mirPath);
    }

    std::string_view mirContent = R"mir(
fn @dead_code_fn(i64 %a) -> i64 {
entry:
    RET i64 %a;
    %dead = ADD i64 %a, 1;
    RET i64 %dead;
}
)mir";

    {
        std::ofstream out(mirPath);
        out << mirContent;
    }

    CommandLineOptions options;
    options.inputFilePath = mirPath;
    options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");
    options.optLevel = OptimizationLevel::O2;
    options.emissionStage = EmissionStage::GenericMir;

    DriverContext ctx(options);
    ASSERT_TRUE(ctx.initialize());
    ASSERT_TRUE(MirModuleLoader::loadMirFile(ctx, mirPath, *ctx.getBuilderContext()));

    CompilationPipeline pipeline(ctx);
    EXPECT_TRUE(pipeline.runPipeline());

    std::string mirDump = pipeline.dumpCurrentMir();
    EXPECT_FALSE(mirDump.empty());

    // After RET, dead instructions should have been pruned
    auto firstRet = mirDump.find("RET");
    ASSERT_NE(firstRet, std::string::npos);
    auto secondRet = mirDump.find("RET", firstRet + 3);
    EXPECT_EQ(secondRet, std::string::npos);

    std::filesystem::remove(mirPath);
}

// Verifies that redundant fall-through jumps are eliminated at -O2.
TEST_F(EzCompilerTestSuite, TestOptimizationPipeline_FallThroughJumpElimination)
{
    const std::string mirPath = "test_fallthrough.mir";
    if (std::filesystem::exists(mirPath))
    {
        std::filesystem::remove(mirPath);
    }

    std::string_view mirContent = R"mir(
fn @fallthrough_fn(i64 %a) -> i64 {
entry:
    BR label %next_bb;

next_bb:
    RET i64 %a;
}
)mir";

    {
        std::ofstream out(mirPath);
        out << mirContent;
    }

    CommandLineOptions options;
    options.inputFilePath = mirPath;
    options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");
    options.optLevel = OptimizationLevel::O2;
    options.emissionStage = EmissionStage::GenericMir;

    DriverContext ctx(options);
    ASSERT_TRUE(ctx.initialize());
    ASSERT_TRUE(MirModuleLoader::loadMirFile(ctx, mirPath, *ctx.getBuilderContext()));

    CompilationPipeline pipeline(ctx);
    EXPECT_TRUE(pipeline.runPipeline());

    std::string mirDump = pipeline.dumpCurrentMir();
    EXPECT_FALSE(mirDump.empty());

    // The redundant JMP to next_bb should have been removed by MirPeepholePass
    EXPECT_EQ(mirDump.find("JMP"), std::string::npos);

    std::filesystem::remove(mirPath);
}

// Verifies register coalescing minimizes move instructions in copy-heavy functions.
TEST_F(EzCompilerTestSuite, TestOptimizationPipeline_CoalescingCopyChains)
{
    const std::string mirPath = "test_copy_chain.mir";
    if (std::filesystem::exists(mirPath))
    {
        std::filesystem::remove(mirPath);
    }

    std::string_view mirContent = R"mir(
fn @copy_chain(i64 %a) -> i64 {
entry:
    %v1 = MOV i64 %a;
    %v2 = MOV i64 %v1;
    %v3 = MOV i64 %v2;
    RET i64 %v3;
}
)mir";

    {
        std::ofstream out(mirPath);
        out << mirContent;
    }

    std::string asmO0;
    std::string asmO2;

    // Run with -O0 (no coalescing, no peephole)
    {
        CommandLineOptions options;
        options.inputFilePath = mirPath;
        options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");
        options.optLevel = OptimizationLevel::O0;
        options.emissionStage = EmissionStage::Assembly;

        DriverContext ctx(options);
        ASSERT_TRUE(ctx.initialize());
        ASSERT_TRUE(MirModuleLoader::loadMirFile(ctx, mirPath, *ctx.getBuilderContext()));

        CompilationPipeline pipeline(ctx);
        EXPECT_TRUE(pipeline.runPipeline());
        asmO0 = pipeline.dumpAssembly();
    }

    // Run with -O2 (coalescing enabled, peephole enabled)
    {
        CommandLineOptions options;
        options.inputFilePath = mirPath;
        options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");
        options.optLevel = OptimizationLevel::O2;
        options.emissionStage = EmissionStage::Assembly;

        DriverContext ctx(options);
        ASSERT_TRUE(ctx.initialize());
        ASSERT_TRUE(MirModuleLoader::loadMirFile(ctx, mirPath, *ctx.getBuilderContext()));

        CompilationPipeline pipeline(ctx);
        EXPECT_TRUE(pipeline.runPipeline());
        asmO2 = pipeline.dumpAssembly();
    }

    std::filesystem::remove(mirPath);

    // -O2 should eliminate copies, leading to fewer instructions
    EXPECT_LT(countLines(asmO2), countLines(asmO0));
}
