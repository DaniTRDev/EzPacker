#include "EzCompilerTestSuite.h"
#include "FrontendAdapter.h"
#include "CompilationPipeline.h"
#include "EmissionEngine.h"
#include <fstream>
#include <filesystem>
#include <vector>

using namespace EzCompiler;

class CFrontendEndToEndTest : public EzCompilerTestSuite
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

  private:
    std::vector<std::string> m_cleanupPaths;
};

// Tests full end-to-end compilation of a .c source file to an ELF64 object file.
TEST_F(CFrontendEndToEndTest, TestFullCompilationCSourceToElf64)
{
    const std::string cPath = "test_c_source.c";
    const std::string outPath = "test_c_elf.o";
    registerCleanup(cPath);
    registerCleanup(outPath);

    {
        std::ofstream out(cPath);
        out << "int main() { return 42; }\n";
    }

    CommandLineOptions options;
    options.inputFilePath = cPath;
    options.outputFilePath = outPath;
    options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");
    options.optLevel = OptimizationLevel::O0;
    options.emissionStage = EmissionStage::Object;

    DriverContext ctx(options);
    ASSERT_TRUE(ctx.initialize());

    MirModuleLoader loader;
    ASSERT_TRUE(loader.compileSourceToMir(ctx, cPath, *ctx.getBuilderContext()));

    CompilationPipeline pipeline(ctx);
    EXPECT_TRUE(pipeline.runPipeline());

    EmissionEngine emitter(ctx);
    EXPECT_TRUE(emitter.emitModule(*ctx.getBuilderContext(), outPath));

    ASSERT_TRUE(std::filesystem::exists(outPath));
    uintmax_t fileSize = std::filesystem::file_size(outPath);
    ASSERT_GE(fileSize, 64u);

    std::ifstream inFile(outPath, std::ios::binary);
    ASSERT_TRUE(inFile.is_open());

    std::vector<uint8_t> header(64);
    inFile.read(reinterpret_cast<char *>(header.data()), 64);
    inFile.close();

    // Verify ELF header
    EXPECT_EQ(header[0], 0x7F);
    EXPECT_EQ(header[1], 'E');
    EXPECT_EQ(header[2], 'L');
    EXPECT_EQ(header[3], 'F');
    EXPECT_EQ(header[4], 2); // 64-bit
    EXPECT_EQ(header[5], 1); // Little-endian
}

// Tests full end-to-end compilation of a .c source file to a COFF object file for Windows.
TEST_F(CFrontendEndToEndTest, TestFullCompilationCSourceToCoff)
{
    const std::string cPath = "test_c_calc.c";
    const std::string outPath = "test_c_coff.obj";
    registerCleanup(cPath);
    registerCleanup(outPath);

    {
        std::ofstream out(cPath);
        out << R"c(
            int calc(int a, int b) {
                int res = (a + b) * 2;
                return res;
            }
        )c";
    }

    CommandLineOptions options;
    options.inputFilePath = cPath;
    options.outputFilePath = outPath;
    options.target = TargetTriple::parse("x86_64-pc-windows-msvc");
    options.optLevel = OptimizationLevel::O0;
    options.emissionStage = EmissionStage::Object;

    DriverContext ctx(options);
    ASSERT_TRUE(ctx.initialize());

    MirModuleLoader loader;
    ASSERT_TRUE(loader.compileSourceToMir(ctx, cPath, *ctx.getBuilderContext()));

    CompilationPipeline pipeline(ctx);
    EXPECT_TRUE(pipeline.runPipeline());

    EmissionEngine emitter(ctx);
    EXPECT_TRUE(emitter.emitModule(*ctx.getBuilderContext(), outPath));

    ASSERT_TRUE(std::filesystem::exists(outPath));
    uintmax_t fileSize = std::filesystem::file_size(outPath);
    ASSERT_GE(fileSize, 20u);

    std::ifstream inFile(outPath, std::ios::binary);
    ASSERT_TRUE(inFile.is_open());

    std::vector<uint8_t> header(20);
    inFile.read(reinterpret_cast<char *>(header.data()), 20);
    inFile.close();

    uint16_t machine = 0;
    std::memcpy(&machine, &header[0], 2);
    EXPECT_EQ(machine, 0x8664); // IMAGE_FILE_MACHINE_AMD64
}

// Tests C compilation with preprocessor includes (-I) and macro definitions (-D).
TEST_F(CFrontendEndToEndTest, TestCSourceWithPreprocessorMacrosAndIncludes)
{
    const std::string incDir = "test_c_inc_dir";
    const std::string headerPath = incDir + "/test_header.h";
    const std::string cPath = "test_macro.c";
    const std::string outPath = "test_macro.o";

    std::error_code ec;
    std::filesystem::create_directories(incDir, ec);

    registerCleanup(cPath);
    registerCleanup(outPath);
    registerCleanup(headerPath);
    registerCleanup(incDir);

    {
        std::ofstream hOut(headerPath);
        hOut << "#define HEADER_CONST 5\n";
    }

    {
        std::ofstream cOut(cPath);
        cOut << R"c(
            #include "test_header.h"
            #ifndef MULTIPLIER
            #define MULTIPLIER 1
            #endif

            int compute(int x) {
                return (x + HEADER_CONST) * MULTIPLIER;
            }
        )c";
    }

    CommandLineOptions options;
    options.inputFilePath = cPath;
    options.outputFilePath = outPath;
    options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");
    options.includePaths.push_back(incDir);
    options.macroDefines.push_back("MULTIPLIER=10");
    options.emissionStage = EmissionStage::Assembly;

    DriverContext ctx(options);
    ASSERT_TRUE(ctx.initialize());

    MirModuleLoader loader;
    ASSERT_TRUE(loader.compileSourceToMir(ctx, cPath, *ctx.getBuilderContext()));

    CompilationPipeline pipeline(ctx);
    EXPECT_TRUE(pipeline.runPipeline());

    std::string asmOutput = pipeline.dumpAssembly();
    EXPECT_FALSE(asmOutput.empty());
    EXPECT_NE(asmOutput.find("compute"), std::string::npos);
}

// Tests C loops and conditionals through full O2 optimization pipeline.
TEST_F(CFrontendEndToEndTest, TestCSourceLoopsAndOptimizationO2)
{
    const std::string cPath = "test_fib.c";
    registerCleanup(cPath);

    {
        std::ofstream out(cPath);
        out << R"c(
            int fib(int n) {
                int a = 0;
                int b = 1;
                for (int i = 0; i < n; i = i + 1) {
                    int c = a + b;
                    a = b;
                    b = c;
                }
                return a;
            }
        )c";
    }

    CommandLineOptions options;
    options.inputFilePath = cPath;
    options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");
    options.optLevel = OptimizationLevel::O2;
    options.emissionStage = EmissionStage::Assembly;

    DriverContext ctx(options);
    ASSERT_TRUE(ctx.initialize());

    MirModuleLoader loader;
    ASSERT_TRUE(loader.compileSourceToMir(ctx, cPath, *ctx.getBuilderContext()));

    CompilationPipeline pipeline(ctx);
    EXPECT_TRUE(pipeline.runPipeline());

    std::string asmOutput = pipeline.dumpAssembly();
    EXPECT_FALSE(asmOutput.empty());
    EXPECT_NE(asmOutput.find("fib"), std::string::npos);
}

// Tests full end-to-end compilation of a C source file including freestanding headers to ELF64 object.
TEST_F(CFrontendEndToEndTest, TestFreestandingHeadersEndToEndToElf)
{
    const std::string cPath = "test_freestanding_elf.c";
    const std::string outPath = "test_freestanding_elf.o";
    registerCleanup(cPath);
    registerCleanup(outPath);

    {
        std::ofstream out(cPath);
        out << R"c(
            #include <stdint.h>
            #include <stddef.h>
            #include <stdbool.h>
            #include <limits.h>

            uint32_t process_stream(const uint8_t *buffer, size_t len) {
                if (buffer == NULL || len == 0)
                    return 0;

                uint32_t checksum = 0;
                for (size_t i = 0; i < len; i = i + 1) {
                    checksum = checksum + (uint32_t)buffer[i];
                }
                return checksum;
            }
        )c";
    }

    CommandLineOptions options;
    options.inputFilePath = cPath;
    options.outputFilePath = outPath;
    options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");
    options.optLevel = OptimizationLevel::O1;
    options.emissionStage = EmissionStage::Object;

    DriverContext ctx(options);
    ASSERT_TRUE(ctx.initialize());

    MirModuleLoader loader;
    ASSERT_TRUE(loader.compileSourceToMir(ctx, cPath, *ctx.getBuilderContext()));

    CompilationPipeline pipeline(ctx);
    EXPECT_TRUE(pipeline.runPipeline());

    EmissionEngine emitter(ctx);
    EXPECT_TRUE(emitter.emitModule(*ctx.getBuilderContext(), outPath));

    ASSERT_TRUE(std::filesystem::exists(outPath));
    uintmax_t fileSize = std::filesystem::file_size(outPath);
    ASSERT_GE(fileSize, 64u);

    std::ifstream inFile(outPath, std::ios::binary);
    ASSERT_TRUE(inFile.is_open());

    std::vector<uint8_t> header(64);
    inFile.read(reinterpret_cast<char *>(header.data()), 64);
    inFile.close();

    // Verify ELF header
    EXPECT_EQ(header[0], 0x7F);
    EXPECT_EQ(header[1], 'E');
    EXPECT_EQ(header[2], 'L');
    EXPECT_EQ(header[3], 'F');
    EXPECT_EQ(header[4], 2); // 64-bit
    EXPECT_EQ(header[5], 1); // Little-endian
}

