#include "EzCompilerTestSuite.h"
#include "FrontendAdapter.h"
#include "CompilationPipeline.h"
#include "EmissionEngine.h"
#include <fstream>
#include <filesystem>
#include <vector>

using namespace EzCompiler;

class CBuiltinsAndHeadersTest : public EzCompilerTestSuite
{
  protected:
    void TearDown() override
    {
        for (const auto &p : m_cleanupPaths)
        {
            std::error_code ec;
            std::filesystem::remove_all(p, ec);
        }
        EzCompilerTestSuite::TearDown();
    }

    void registerCleanup(const std::string &path)
    {
        m_cleanupPaths.push_back(path);
    }

    std::string findExamplePath(const std::string &relPath)
    {
        std::vector<std::string> prefixes = {"", "../", "../../", "../../../", "../../../../"};
        for (const auto &p : prefixes)
        {
            if (std::filesystem::exists(p + relPath))
                return std::filesystem::canonical(p + relPath).string();
        }
        return relPath;
    }

  private:
    std::vector<std::string> m_cleanupPaths;
};

// Tests compilation of standard headers without any explicit -I flags
TEST_F(CBuiltinsAndHeadersTest, TestAutoConfiguredStandardHeaders)
{
    const std::string cPath = "test_headers.c";
    registerCleanup(cPath);

    {
        std::ofstream out(cPath);
        out << R"c(
            #include <stdint.h>
            #include <stdbool.h>
            #include <stddef.h>

            int64_t compute_sizes(size_t len) {
                int32_t a = 10;
                uint64_t b = 20;
                bool flag = (len > 0);
                if (flag) {
                    return (int64_t)(a + b + len);
                }
                return 0;
            }
        )c";
    }

    CommandLineOptions options;
    options.inputFilePath = cPath;
    options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");
    options.optLevel = OptimizationLevel::O0;
    options.emissionStage = EmissionStage::Assembly;

    DriverContext ctx(options);
    ASSERT_TRUE(ctx.initialize());

    MirModuleLoader loader;
    ASSERT_TRUE(loader.compileSourceToMir(ctx, cPath, *ctx.getBuilderContext()));

    CompilationPipeline pipeline(ctx);
    EXPECT_TRUE(pipeline.runPipeline());

    std::string asmOut = pipeline.dumpAssembly();
    EXPECT_FALSE(asmOut.empty());
    EXPECT_NE(asmOut.find("compute_sizes"), std::string::npos);
}

// Tests GCC/Clang builtins: __builtin_expect, __builtin_bswap*, __builtin_constant_p, __builtin_unreachable
TEST_F(CBuiltinsAndHeadersTest, TestGccClangBuiltins)
{
    const std::string cPath = "test_builtins.c";
    registerCleanup(cPath);

    {
        std::ofstream out(cPath);
        out << R"c(
            #include <stdint.h>

            uint32_t test_bswaps(uint32_t val) {
                if (__builtin_expect(val != 0, 1)) {
                    uint16_t s16 = __builtin_bswap16((uint16_t)val);
                    uint32_t s32 = __builtin_bswap32(val);
                    uint64_t s64 = __builtin_bswap64((uint64_t)val);
                    int is_c = __builtin_constant_p(100);
                    return s32 + s16 + (uint32_t)s64 + is_c;
                }
                __builtin_unreachable();
                return 0;
            }
        )c";
    }

    CommandLineOptions options;
    options.inputFilePath = cPath;
    options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");
    options.optLevel = OptimizationLevel::O1;
    options.emissionStage = EmissionStage::Assembly;

    DriverContext ctx(options);
    ASSERT_TRUE(ctx.initialize());

    MirModuleLoader loader;
    ASSERT_TRUE(loader.compileSourceToMir(ctx, cPath, *ctx.getBuilderContext()));

    CompilationPipeline pipeline(ctx);
    EXPECT_TRUE(pipeline.runPipeline());

    std::string asmOut = pipeline.dumpAssembly();
    EXPECT_FALSE(asmOut.empty());
    EXPECT_NE(asmOut.find("test_bswaps"), std::string::npos);
}

// Tests C23 / AVX-512 extended floating-point suffixes (f16, bf16, f32, f64)
TEST_F(CBuiltinsAndHeadersTest, TestC23FloatLiteralSuffixes)
{
    const std::string cPath = "test_f16.c";
    registerCleanup(cPath);

    {
        std::ofstream out(cPath);
        out << R"c(
            float test_f16(void) {
                float h = 0.0f16;
                float f = 1.5f32;
                double d = 2.5f64;
                if (h < f) {
                    return (float)(f + d);
                }
                return h;
            }
        )c";
    }

    CommandLineOptions options;
    options.inputFilePath = cPath;
    options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");
    options.optLevel = OptimizationLevel::O0;
    options.emissionStage = EmissionStage::Assembly;

    DriverContext ctx(options);
    ASSERT_TRUE(ctx.initialize());

    MirModuleLoader loader;
    ASSERT_TRUE(loader.compileSourceToMir(ctx, cPath, *ctx.getBuilderContext()));

    CompilationPipeline pipeline(ctx);
    EXPECT_TRUE(pipeline.runPipeline());

    std::string asmOut = pipeline.dumpAssembly();
    EXPECT_FALSE(asmOut.empty());
    EXPECT_NE(asmOut.find("test_f16"), std::string::npos);
}

// Tests full variadic arguments with stdarg.h and __builtin_va_list
TEST_F(CBuiltinsAndHeadersTest, TestVarargsAndStdarg)
{
    const std::string cPath = "test_varargs.c";
    registerCleanup(cPath);

    {
        std::ofstream out(cPath);
        out << R"c(
            #include <stdarg.h>

            int sum_ints(int count, ...) {
                va_list ap;
                va_start(ap, count);
                int sum = 0;
                for (int i = 0; i < count; i++) {
                    sum += va_arg(ap, int);
                }
                va_end(ap);
                return sum;
            }
        )c";
    }

    CommandLineOptions options;
    options.inputFilePath = cPath;
    options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");
    options.optLevel = OptimizationLevel::O0;
    options.emissionStage = EmissionStage::Assembly;

    DriverContext ctx(options);
    ASSERT_TRUE(ctx.initialize());

    MirModuleLoader loader;
    ASSERT_TRUE(loader.compileSourceToMir(ctx, cPath, *ctx.getBuilderContext()));

    CompilationPipeline pipeline(ctx);
    EXPECT_TRUE(pipeline.runPipeline());

    std::string asmOut = pipeline.dumpAssembly();
    EXPECT_FALSE(asmOut.empty());
    EXPECT_NE(asmOut.find("sum_ints"), std::string::npos);
}

// Tests compiling real-world freestanding SHA-256 implementation
TEST_F(CBuiltinsAndHeadersTest, TestRealWorldSha256)
{
    CommandLineOptions options;
    options.inputFilePath = findExamplePath("examples/c_sha256.c");
    options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");
    options.optLevel = OptimizationLevel::O2;
    options.emissionStage = EmissionStage::Assembly;

    DriverContext ctx(options);
    ASSERT_TRUE(ctx.initialize());

    MirModuleLoader loader;
    ASSERT_TRUE(loader.compileSourceToMir(ctx, options.inputFilePath, *ctx.getBuilderContext()));

    CompilationPipeline pipeline(ctx);
    EXPECT_TRUE(pipeline.runPipeline());

    std::string asmOut = pipeline.dumpAssembly();
    EXPECT_FALSE(asmOut.empty());
    EXPECT_NE(asmOut.find("sha256_transform"), std::string::npos);
}

// Tests compiling SQLite-style freestanding kernel snippet
TEST_F(CBuiltinsAndHeadersTest, TestRealWorldSqliteSnippet)
{
    CommandLineOptions options;
    options.inputFilePath = findExamplePath("examples/c_sqlite_snippet.c");
    options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");
    options.optLevel = OptimizationLevel::O1;
    options.emissionStage = EmissionStage::Assembly;

    DriverContext ctx(options);
    ASSERT_TRUE(ctx.initialize());

    MirModuleLoader loader;
    ASSERT_TRUE(loader.compileSourceToMir(ctx, options.inputFilePath, *ctx.getBuilderContext()));

    CompilationPipeline pipeline(ctx);
    EXPECT_TRUE(pipeline.runPipeline());

    std::string asmOut = pipeline.dumpAssembly();
    EXPECT_FALSE(asmOut.empty());
    EXPECT_NE(asmOut.find("sqlite3_format_error"), std::string::npos);
}
