#include "EzCompilerTestSuite.h"
#include "FrontendAdapter.h"
#include <fstream>
#include <filesystem>

using namespace EzCompiler;

// Compiles a `return 42` function through the full pipeline to an ELF object and validates the ELF header fields.
TEST_F(EzCompilerTestSuite, TestFullCompilationToElf64)
{
    const std::string outPath = "test_ezc_elf.o";
    if (std::filesystem::exists(outPath))
    {
        std::filesystem::remove(outPath);
    }

    CommandLineOptions options;
    options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");
    options.outputFilePath = outPath;
    options.emissionStage = EmissionStage::Object;

    DriverContext ctx(options);
    ASSERT_TRUE(ctx.initialize());

    // Create function: return 42;
    MirFunction *func = MirModuleLoader::createReturnConstFunction(ctx, "main", 42);
    ASSERT_NE(func, nullptr);

    CompilationPipeline pipeline(ctx);
    EXPECT_TRUE(pipeline.runPipeline());

    EmissionEngine emitter(ctx);
    EXPECT_TRUE(emitter.emitModule(*ctx.getBuilderContext(), outPath));

    ASSERT_TRUE(std::filesystem::exists(outPath));
    uintmax_t fileSize = std::filesystem::file_size(outPath);
    ASSERT_GE(fileSize, 64u);

    // Verify ELF header
    std::ifstream inFile(outPath, std::ios::binary);
    ASSERT_TRUE(inFile.is_open());

    std::vector<uint8_t> header(64);
    inFile.read(reinterpret_cast<char *>(header.data()), 64);
    inFile.close();

    EXPECT_EQ(header[0], 0x7F);
    EXPECT_EQ(header[1], 'E');
    EXPECT_EQ(header[2], 'L');
    EXPECT_EQ(header[3], 'F');
    EXPECT_EQ(header[4], 2); // 64-bit
    EXPECT_EQ(header[5], 1); // Little endian

    uint16_t e_type = 0;
    std::memcpy(&e_type, &header[16], 2);
    EXPECT_EQ(e_type, 1); // ET_REL

    uint16_t e_machine = 0;
    std::memcpy(&e_machine, &header[18], 2);
    EXPECT_EQ(e_machine, 62); // EM_X86_64

    std::filesystem::remove(outPath);
}

// Compiles an arithmetic function through the full pipeline to a COFF object and checks the machine type.
TEST_F(EzCompilerTestSuite, TestFullCompilationToCoff)
{
    const std::string outPath = "test_ezc_coff.obj";
    if (std::filesystem::exists(outPath))
    {
        std::filesystem::remove(outPath);
    }

    CommandLineOptions options;
    options.target = TargetTriple::parse("x86_64-pc-windows-msvc");
    options.outputFilePath = outPath;
    options.emissionStage = EmissionStage::Object;

    DriverContext ctx(options);
    ASSERT_TRUE(ctx.initialize());

    // Create function: 10 + 32 = 42
    MirFunction *func = MirModuleLoader::createArithmeticFunction(ctx, "calc");
    ASSERT_NE(func, nullptr);

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

    std::filesystem::remove(outPath);
}

// Verifies the GenericMir and Assembly stages stop the pipeline at inspectable MIR and assembly output.
TEST_F(EzCompilerTestSuite, TestPipelineInspectionGates)
{
    // 1. Generic MIR Gate
    {
        CommandLineOptions options;
        options.target = TargetTriple::parse("x86_64-linux-gnu");
        options.emissionStage = EmissionStage::GenericMir;

        DriverContext ctx(options);
        ASSERT_TRUE(ctx.initialize());

        MirModuleLoader::createReturnConstFunction(ctx, "test_gate", 100);

        CompilationPipeline pipeline(ctx);
        EXPECT_TRUE(pipeline.runPipeline());

        std::string mir = pipeline.dumpCurrentMir();
        EXPECT_FALSE(mir.empty());
        EXPECT_NE(mir.find("test_gate"), std::string::npos);
    }

    // 2. Assembly Gate
    {
        CommandLineOptions options;
        options.target = TargetTriple::parse("x86_64-linux-gnu");
        options.emissionStage = EmissionStage::Assembly;

        DriverContext ctx(options);
        ASSERT_TRUE(ctx.initialize());

        MirModuleLoader::createReturnConstFunction(ctx, "test_asm", 100);

        CompilationPipeline pipeline(ctx);
        EXPECT_TRUE(pipeline.runPipeline());

        std::string asmStr = pipeline.dumpAssembly();
        EXPECT_FALSE(asmStr.empty());
        EXPECT_NE(asmStr.find(".globl test_asm"), std::string::npos);
    }
}

// Loads a .mir source file, runs the compilation pipeline, and emits an ELF object.
TEST_F(EzCompilerTestSuite, TestCompileMirFileDirectly)
{
    const std::string mirPath = "test_input.mir";
    const std::string outPath = "test_mir_elf.o";
    if (std::filesystem::exists(outPath))
    {
        std::filesystem::remove(outPath);
    }

    std::string_view mirContent = R"mir(
fn @add_func(i64 %a, i64 %b) -> i64 {
entry:
    %sum = ADD i64 %a, %b;
    RET i64 %sum;
}
)mir";

    {
        std::ofstream out(mirPath);
        out << mirContent;
    }

    CommandLineOptions options;
    options.inputFilePath = mirPath;
    options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");
    options.outputFilePath = outPath;
    options.emissionStage = EmissionStage::Object;

    DriverContext ctx(options);
    ASSERT_TRUE(ctx.initialize());

    ASSERT_TRUE(MirModuleLoader::loadMirFile(ctx, mirPath, *ctx.getBuilderContext()));

    CompilationPipeline pipeline(ctx);
    EXPECT_TRUE(pipeline.runPipeline());

    EmissionEngine emitter(ctx);
    EXPECT_TRUE(emitter.emitModule(*ctx.getBuilderContext(), outPath));

    ASSERT_TRUE(std::filesystem::exists(outPath));
    uintmax_t fileSize = std::filesystem::file_size(outPath);
    ASSERT_GE(fileSize, 64u);

    std::filesystem::remove(mirPath);
    std::filesystem::remove(outPath);
}
