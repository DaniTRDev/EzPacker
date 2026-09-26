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

// LEG-08: only .mir inputs are supported; other extensions must be rejected explicitly.
TEST_F(EzCompilerTestSuite, TestUnsupportedSourceFormatRejected)
{
    const std::string srcPath = "test_unsupported_source.ez";
    {
        std::ofstream out(srcPath);
        out << "int main() { return 0; }\n";
    }

    CommandLineOptions options;
    options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");

    DriverContext ctx(options);
    ASSERT_TRUE(ctx.initialize());

    EzCompiler::MirModuleLoader loader;
    EXPECT_FALSE(loader.compileSourceToMir(ctx, srcPath, *ctx.getBuilderContext()));

    std::filesystem::remove(srcPath);
}

// Compiles a module with 128-bit and 256-bit globals through the full pipeline and verifies byte serialization in object output.
TEST_F(EzCompilerTestSuite, TestArbitraryPrecisionGlobalEmission)
{
    const std::string mirPath = "test_big_globals.mir";
    const std::string outPath = "test_big_globals.o";
    const std::string mirContent = R"mir(
@g_imm128 = internal var i128 = 0x112233445566778899aabbccddeeff00;
@g_imm256 = internal var i256 = 0x0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef;

fn @main() -> i64 {
entry:
    %v0 = MOV i64 0;
    RET i64 %v0;
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

    // Read full object file bytes
    std::ifstream inFile(outPath, std::ios::binary);
    ASSERT_TRUE(inFile.is_open());
    std::vector<uint8_t> fileBytes((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    // Verify 16-byte pattern of 0x112233445566778899aabbccddeeff00 is present in full width (not truncated to 8 bytes)
    const std::vector<uint8_t> expected128 = {
        0x00, 0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99,
        0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11
    };

    auto it = std::search(fileBytes.begin(), fileBytes.end(), expected128.begin(), expected128.end());
    EXPECT_NE(it, fileBytes.end()) << "Full 16-byte sequence for 128-bit integer was not found in object output!";

    std::filesystem::remove(mirPath);
    std::filesystem::remove(outPath);
}

// Compiles a module declaring an extern C function (@puts) and calling it,
// verifying undefined symbol emission, internal/weak linkage bindings, and relocations.
TEST_F(EzCompilerTestSuite, TestExternFunctionAndLinkageEndToEnd)
{
    const std::string mirPath = "test_extern_linkage.mir";
    const std::string outPath = "test_extern_linkage.o";
    const std::string mirContent = R"mir(
declare @puts(ptr) -> i32;

internal fn @internal_helper() -> i32 {
entry:
    %v = MOV i32 42;
    RET i32 %v;
}

weak fn @weak_helper() -> i32 {
entry:
    %w = MOV i32 100;
    RET i32 %w;
}

fn @main() -> i32 {
entry:
    %res = CALL i32 @puts;
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

    // Read full object file bytes
    std::ifstream inFile(outPath, std::ios::binary);
    ASSERT_TRUE(inFile.is_open());
    std::vector<uint8_t> fileBytes((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    // Parse ELF64 headers to verify symbol table
    uint64_t e_shoff = 0;
    std::memcpy(&e_shoff, &fileBytes[40], 8);
    uint16_t e_shentsize = 0;
    std::memcpy(&e_shentsize, &fileBytes[58], 2);
    uint16_t e_shnum = 0;
    std::memcpy(&e_shnum, &fileBytes[60], 2);

    ASSERT_GT(e_shoff, 0u);
    ASSERT_GT(e_shnum, 0u);

    uint64_t symtabOffset = 0;
    uint64_t symtabSize = 0;
    uint32_t strtabSecIdx = 0;
    bool foundRelaText = false;

    for (uint16_t i = 0; i < e_shnum; ++i)
    {
        const uint8_t *shdr = &fileBytes[e_shoff + i * e_shentsize];
        uint32_t sh_type = 0;
        std::memcpy(&sh_type, shdr + 4, 4);

        if (sh_type == 2) // SHT_SYMTAB
        {
            std::memcpy(&symtabOffset, shdr + 24, 8);
            std::memcpy(&symtabSize, shdr + 32, 8);
            std::memcpy(&strtabSecIdx, shdr + 40, 4);
        }
        else if (sh_type == 4) // SHT_RELA
        {
            foundRelaText = true;
        }
    }

    ASSERT_GT(symtabOffset, 0u);
    ASSERT_GT(symtabSize, 0u);
    EXPECT_TRUE(foundRelaText);

    const uint8_t *strtabHdr = &fileBytes[e_shoff + strtabSecIdx * e_shentsize];
    uint64_t strtabOffset = 0;
    std::memcpy(&strtabOffset, strtabHdr + 24, 8);

    struct ParsedSym
    {
        std::string name;
        uint8_t bind;
        uint16_t shndx;
    };
    std::vector<ParsedSym> symbols;

    constexpr size_t ELF_SYM_SIZE = 24;
    size_t numSyms = symtabSize / ELF_SYM_SIZE;
    for (size_t i = 0; i < numSyms; ++i)
    {
        const uint8_t *symData = &fileBytes[symtabOffset + i * ELF_SYM_SIZE];
        uint32_t st_name = 0;
        std::memcpy(&st_name, symData, 4);
        uint8_t st_info = symData[4];
        uint16_t st_shndx = 0;
        std::memcpy(&st_shndx, symData + 6, 2);

        const char *namePtr = reinterpret_cast<const char *>(&fileBytes[strtabOffset + st_name]);
        symbols.push_back({ std::string(namePtr), static_cast<uint8_t>(st_info >> 4), st_shndx });
    }

    auto findSym = [&](std::string_view name) -> const ParsedSym * {
        for (const auto &s : symbols)
        {
            if (s.name == name) return &s;
        }
        return nullptr;
    };

    // Verify puts is undefined external function
    const ParsedSym *psPuts = findSym("puts");
    ASSERT_NE(psPuts, nullptr);
    EXPECT_EQ(psPuts->bind, 1); // STB_GLOBAL
    EXPECT_EQ(psPuts->shndx, 0); // SHN_UNDEF

    // Verify internal_helper is local
    const ParsedSym *psInternal = findSym("internal_helper");
    ASSERT_NE(psInternal, nullptr);
    EXPECT_EQ(psInternal->bind, 0); // STB_LOCAL
    EXPECT_NE(psInternal->shndx, 0); // defined in .text

    // Verify weak_helper is weak
    const ParsedSym *psWeak = findSym("weak_helper");
    ASSERT_NE(psWeak, nullptr);
    EXPECT_EQ(psWeak->bind, 2); // STB_WEAK
    EXPECT_NE(psWeak->shndx, 0); // defined in .text

    // Verify main is global
    const ParsedSym *psMain = findSym("main");
    ASSERT_NE(psMain, nullptr);
    EXPECT_EQ(psMain->bind, 1); // STB_GLOBAL
    EXPECT_NE(psMain->shndx, 0); // defined in .text

    std::filesystem::remove(mirPath);
    std::filesystem::remove(outPath);
}

