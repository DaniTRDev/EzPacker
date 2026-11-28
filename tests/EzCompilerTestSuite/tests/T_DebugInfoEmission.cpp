#include "EzCompilerTestSuite.h"
#include "CommandLineOptions.h"
#include "DriverContext.h"
#include "FrontendAdapter.h"
#include "CompilationPipeline.h"
#include "EmissionEngine.h"
#include "EzLinkerDriver.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace EzCompiler;

namespace
{

std::string runShellCapture(const std::string &cmd, int &outExitCode)
{
    std::string result;
    std::array<char, 4096> buf{};
    FILE *pipe = popen((cmd + " 2>&1").c_str(), "r");
    if (!pipe)
    {
        outExitCode = -1;
        return result;
    }
    while (fgets(buf.data(), static_cast<int>(buf.size()), pipe) != nullptr)
    {
        result += buf.data();
    }
    int status = pclose(pipe);
    outExitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return result;
}

std::filesystem::path getEzcExecutablePath()
{
    std::error_code ec;
    std::filesystem::path selfDir = std::filesystem::read_symlink("/proc/self/exe", ec).parent_path();
    if (!ec && std::filesystem::exists(selfDir / "ezc"))
    {
        return selfDir / "ezc";
    }
    std::filesystem::path candidate = std::filesystem::current_path() / "ezc";
    if (std::filesystem::exists(candidate))
    {
        return candidate;
    }
    candidate = std::filesystem::current_path() / "bin" / "ezc";
    if (std::filesystem::exists(candidate))
    {
        return candidate;
    }
    return "ezc";
}

bool readBinaryFile(const std::filesystem::path &path, std::vector<uint8_t> &out)
{
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.is_open())
    {
        return false;
    }
    const auto sz = in.tellg();
    if (sz < 0)
    {
        return false;
    }
    in.seekg(0, std::ios::beg);
    out.resize(static_cast<size_t>(sz));
    return in.read(reinterpret_cast<char *>(out.data()), sz).good();
}

uint16_t readU16(const std::vector<uint8_t> &b, size_t off)
{
    return static_cast<uint16_t>(b[off]) | (static_cast<uint16_t>(b[off + 1]) << 8);
}

uint32_t readU32(const std::vector<uint8_t> &b, size_t off)
{
    return static_cast<uint32_t>(b[off]) | (static_cast<uint32_t>(b[off + 1]) << 8) |
           (static_cast<uint32_t>(b[off + 2]) << 16) | (static_cast<uint32_t>(b[off + 3]) << 24);
}

bool compileCSourceInProcess(const std::string &cSourcePath,
                             const std::string &outObjPath,
                             const std::string &tripleStr,
                             EzCore::Debug::DebugLevel level = EzCore::Debug::DebugLevel::Full,
                             EzCore::Debug::DebugFormat format = EzCore::Debug::DebugFormat::Auto,
                             const std::string &telemetryJsonPath = {})
{
    CommandLineOptions options;
    options.inputFilePath = cSourcePath;
    options.outputFilePath = outObjPath;
    options.target = TargetTriple::parse(tripleStr);
    options.emissionStage = EmissionStage::Object;
    options.optLevel = OptimizationLevel::O0;
    options.debugLevel = level;
    options.debugFormat = format;
    options.pipelineDumpJsonPath = telemetryJsonPath;

    DriverContext ctx(options);
    if (!ctx.initialize())
    {
        return false;
    }

    CompilationPipeline pipeline(ctx);
    MirModuleLoader loader;
    if (!loader.compileSourceToMir(ctx, cSourcePath, *ctx.getBuilderContext()))
    {
        return false;
    }
    if (!pipeline.runPipeline())
    {
        return false;
    }

    EmissionEngine emitter(ctx);
    if (!emitter.emitModule(*ctx.getBuilderContext(), outObjPath))
    {
        return false;
    }
    pipeline.writeTelemetryIfRequested();
    return true;
}

} // namespace

TEST_F(EzCompilerTestSuite, DebugCliFlagsAndConflicts)
{
    CommandLineParser parser;
    CommandLineOptions opts;
    std::string err;

    EXPECT_TRUE(parser.parse({ "ezc", "-g", "a.c" }, opts, err));
    EXPECT_EQ(opts.debugLevel, EzCore::Debug::DebugLevel::Full);
    EXPECT_EQ(opts.debugFormat, EzCore::Debug::DebugFormat::Auto);
    EXPECT_TRUE(opts.emitDebugInfo());

    EXPECT_TRUE(parser.parse({ "ezc", "--debug-info", "a.c" }, opts, err));
    EXPECT_EQ(opts.debugLevel, EzCore::Debug::DebugLevel::Full);

    EXPECT_TRUE(parser.parse({ "ezc", "-g0", "a.c" }, opts, err));
    EXPECT_EQ(opts.debugLevel, EzCore::Debug::DebugLevel::None);
    EXPECT_FALSE(opts.emitDebugInfo());

    EXPECT_TRUE(parser.parse({ "ezc", "-g1", "--debug-format", "dwarf", "a.c" }, opts, err));
    EXPECT_EQ(opts.debugLevel, EzCore::Debug::DebugLevel::LineTablesOnly);
    EXPECT_EQ(opts.debugFormat, EzCore::Debug::DebugFormat::Dwarf5);

    EXPECT_TRUE(parser.parse({ "ezc", "-g2", "--debug-format", "codeview", "a.c" }, opts, err));
    EXPECT_EQ(opts.debugLevel, EzCore::Debug::DebugLevel::Full);
    EXPECT_EQ(opts.debugFormat, EzCore::Debug::DebugFormat::CodeView);

    EXPECT_FALSE(parser.parse({ "ezc", "-g", "-g0", "a.c" }, opts, err));
    EXPECT_FALSE(err.empty());

    EXPECT_FALSE(parser.parse({ "ezc", "-g1", "-g2", "a.c" }, opts, err));
    EXPECT_FALSE(err.empty());

    EXPECT_FALSE(parser.parse({ "ezc", "-g", "--debug-format", "pdb_invalid", "a.c" }, opts, err));
    EXPECT_FALSE(err.empty());
}

TEST_F(EzCompilerTestSuite, C11X86_64Elf64DwarfValidationAndTelemetry)
{
    const std::filesystem::path srcPath = "t_dbg_x64.c";
    const std::filesystem::path objPath = "t_dbg_x64.o";
    const std::filesystem::path jsonPath = "t_dbg_x64_telemetry.json";
    std::filesystem::remove(srcPath);
    std::filesystem::remove(objPath);
    std::filesystem::remove(jsonPath);

    {
        std::ofstream out(srcPath);
        out << "struct Pair {\n"
            << "    int a;\n"
            << "    long b;\n"
            << "};\n"
            << "int g_counter = 7;\n"
            << "int sum_pair(struct Pair p, int extra) {\n"
            << "    int total = p.a + (int)p.b + extra + g_counter;\n"
            << "    return total;\n"
            << "}\n"
            << "int main(void) {\n"
            << "    struct Pair p;\n"
            << "    p.a = 10;\n"
            << "    p.b = 20;\n"
            << "    return sum_pair(p, 5);\n"
            << "}\n";
    }

    ASSERT_TRUE(compileCSourceInProcess(srcPath.string(),
                                        objPath.string(),
                                        "x86_64-unknown-linux-gnu",
                                        EzCore::Debug::DebugLevel::Full,
                                        EzCore::Debug::DebugFormat::Auto,
                                        jsonPath.string()));
    ASSERT_TRUE(std::filesystem::exists(objPath));
    ASSERT_TRUE(std::filesystem::exists(jsonPath));

    int exitCode = -1;
    std::string dump = runShellCapture("readelf --debug-dump=info,line,frames " + objPath.string(), exitCode);
    EXPECT_EQ(exitCode, 0) << dump;
    EXPECT_NE(dump.find("DW_TAG_compile_unit"), std::string::npos);
    EXPECT_NE(dump.find("DW_TAG_subprogram"), std::string::npos);
    EXPECT_NE(dump.find("sum_pair"), std::string::npos);
    EXPECT_NE(dump.find("Pair"), std::string::npos);
    EXPECT_NE(dump.find("DW_TAG_variable"), std::string::npos);
    EXPECT_NE(dump.find("g_counter"), std::string::npos);
    EXPECT_NE(dump.find("Line Number Statements"), std::string::npos);

    std::ifstream jf(jsonPath);
    std::stringstream jbuf;
    jbuf << jf.rdbuf();
    std::string jsonStr = jbuf.str();
    EXPECT_NE(jsonStr.find("\"debugInfo\""), std::string::npos);
    EXPECT_NE(jsonStr.find("\"enabled\": true"), std::string::npos);
    EXPECT_NE(jsonStr.find("\"format\": \"Dwarf5\""), std::string::npos);
    EXPECT_NE(jsonStr.find(".debug_info"), std::string::npos);

    std::filesystem::remove(srcPath);
    std::filesystem::remove(objPath);
    std::filesystem::remove(jsonPath);
}

TEST_F(EzCompilerTestSuite, C11RiscV64Elf64DwarfValidationViaCli)
{
    const std::filesystem::path srcPath = "t_dbg_rv64.c";
    const std::filesystem::path objPath = "t_dbg_rv64.o";
    std::filesystem::remove(srcPath);
    std::filesystem::remove(objPath);

    {
        std::ofstream out(srcPath);
        out << "int add_vals(int x, int y) {\n"
            << "    int z = x + y;\n"
            << "    return z;\n"
            << "}\n"
            << "int main(void) {\n"
            << "    return add_vals(19, 23);\n"
            << "}\n";
    }

    const std::filesystem::path ezc = getEzcExecutablePath();
    int exitCode = -1;
    std::string cliOut = runShellCapture(
            "\"" + ezc.string() + "\" -g --target riscv64-unknown-linux-gnu -o " + objPath.string() + " " +
                    srcPath.string(),
            exitCode);
    ASSERT_EQ(exitCode, 0) << cliOut;
    ASSERT_TRUE(std::filesystem::exists(objPath));

    std::string dump = runShellCapture("readelf --debug-dump=info,line,frames " + objPath.string(), exitCode);
    EXPECT_EQ(exitCode, 0) << dump;
    EXPECT_NE(dump.find("DW_TAG_compile_unit"), std::string::npos);
    EXPECT_NE(dump.find("add_vals"), std::string::npos);
    EXPECT_NE(dump.find("Line Number Statements"), std::string::npos);

    std::filesystem::remove(srcPath);
    std::filesystem::remove(objPath);
}

TEST_F(EzCompilerTestSuite, C11WindowsCoffCodeViewValidation)
{
    const std::filesystem::path srcPath = "t_dbg_win64.c";
    const std::filesystem::path objPath = "t_dbg_win64.obj";
    std::filesystem::remove(srcPath);
    std::filesystem::remove(objPath);

    {
        std::ofstream out(srcPath);
        out << "int g_win = 11;\n"
            << "int compute(int a, int b) {\n"
            << "    int c = a * b + g_win;\n"
            << "    return c;\n"
            << "}\n";
    }

    ASSERT_TRUE(compileCSourceInProcess(srcPath.string(),
                                        objPath.string(),
                                        "x86_64-pc-windows-msvc",
                                        EzCore::Debug::DebugLevel::Full,
                                        EzCore::Debug::DebugFormat::Auto));

    std::vector<uint8_t> bytes;
    ASSERT_TRUE(readBinaryFile(objPath, bytes));
    ASSERT_GE(bytes.size(), 20u);

    const uint16_t numSections = readU16(bytes, 2);
    const uint16_t optHdrSize = readU16(bytes, 16);
    const size_t secTableOff = 20 + optHdrSize;
    ASSERT_GE(bytes.size(), secTableOff + static_cast<size_t>(numSections) * 40);

    bool foundDebugT = false;
    bool foundDebugS = false;
    bool foundDebugSRelocs = false;
    for (uint16_t i = 0; i < numSections; ++i)
    {
        const size_t off = secTableOff + static_cast<size_t>(i) * 40;
        char nameBuf[9] = {};
        std::memcpy(nameBuf, bytes.data() + off, 8);
        std::string secName(nameBuf);
        const uint32_t rawSize = readU32(bytes, off + 16);
        const uint16_t numRelocs = readU16(bytes, off + 32);
        if (secName == ".debug$T")
        {
            foundDebugT = true;
            EXPECT_GT(rawSize, 4u);
        }
        else if (secName == ".debug$S")
        {
            foundDebugS = true;
            EXPECT_GT(rawSize, 4u);
            if (numRelocs > 0)
            {
                foundDebugSRelocs = true;
            }
        }
    }

    EXPECT_TRUE(foundDebugT);
    EXPECT_TRUE(foundDebugS);
    EXPECT_TRUE(foundDebugSRelocs);

    std::filesystem::remove(srcPath);
    std::filesystem::remove(objPath);
}

TEST_F(EzCompilerTestSuite, MirInputDebugInfoForElfAndCoff)
{
    const std::filesystem::path mirPath = "t_dbg_mir.mir";
    const std::filesystem::path elfObj = "t_dbg_mir.o";
    const std::filesystem::path coffObj = "t_dbg_mir.obj";
    std::filesystem::remove(mirPath);
    std::filesystem::remove(elfObj);
    std::filesystem::remove(coffObj);

    {
        std::ofstream out(mirPath);
        out << "fn @mir_add(i64 %a, i64 %b) -> i64 {\n"
            << "entry:\n"
            << "  %sum = ADD i64 %a, %b;\n"
            << "  RET i64 %sum;\n"
            << "}\n";
    }

    ASSERT_TRUE(compileCSourceInProcess(mirPath.string(),
                                        elfObj.string(),
                                        "x86_64-unknown-linux-gnu",
                                        EzCore::Debug::DebugLevel::Full,
                                        EzCore::Debug::DebugFormat::Dwarf5));
    int exitCode = -1;
    std::string dump = runShellCapture("readelf --debug-dump=info,line " + elfObj.string(), exitCode);
    EXPECT_EQ(exitCode, 0) << dump;
    EXPECT_NE(dump.find("mir_add"), std::string::npos);

    ASSERT_TRUE(compileCSourceInProcess(mirPath.string(),
                                        coffObj.string(),
                                        "x86_64-pc-windows-msvc",
                                        EzCore::Debug::DebugLevel::Full,
                                        EzCore::Debug::DebugFormat::CodeView));
    std::vector<uint8_t> bytes;
    ASSERT_TRUE(readBinaryFile(coffObj, bytes));
    ASSERT_GE(bytes.size(), 20u);
    const std::string rawStr(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    EXPECT_NE(rawStr.find(".debug$T"), std::string::npos);
    EXPECT_NE(rawStr.find(".debug$S"), std::string::npos);

    std::filesystem::remove(mirPath);
    std::filesystem::remove(elfObj);
    std::filesystem::remove(coffObj);
}

TEST_F(EzCompilerTestSuite, RealLinuxExecutableGdbDebugSession)
{
    const std::filesystem::path srcPath = "t_dbg_gdb.c";
    const std::filesystem::path objPath = "t_dbg_gdb.o";
    const std::filesystem::path exePath = "t_dbg_gdb.out";
    std::filesystem::remove(srcPath);
    std::filesystem::remove(objPath);
    std::filesystem::remove(exePath);

    {
        std::ofstream out(srcPath);
        out << "struct Point {\n" // line 1
            << "    int x;\n"      // line 2
            << "    int y;\n"      // line 3
            << "};\n"              // line 4
            << "int compute(int a, int b) {\n" // line 5
            << "    struct Point pt;\n"        // line 6
            << "    pt.x = a;\n"               // line 7
            << "    pt.y = b;\n"               // line 8
            << "    int sum = pt.x + pt.y;\n"  // line 9
            << "    return sum;\n"             // line 10
            << "}\n"                           // line 11
            << "int main(void) {\n"            // line 12
            << "    return compute(17, 25);\n" // line 13
            << "}\n";                          // line 14
    }

    ASSERT_TRUE(compileCSourceInProcess(srcPath.string(),
                                        objPath.string(),
                                        "x86_64-unknown-linux-gnu",
                                        EzCore::Debug::DebugLevel::Full,
                                        EzCore::Debug::DebugFormat::Dwarf5));

    EzLinker::EzLinkerDriver linker;
    EzLinker::LinkerOptions linkOpts;
    linkOpts.inputFiles = { objPath.string() };
    linkOpts.outputFile = exePath.string();
    linkOpts.targetTriple = "x86_64-unknown-linux-gnu";
    linkOpts.isHosted = true;
    linkOpts.emitDebugInfo = true;
    auto linkRes = linker.link(linkOpts);
    ASSERT_TRUE(linkRes.success) << linkRes.output;
    ASSERT_TRUE(std::filesystem::exists(exePath));

    int exitCode = -1;
    std::string gdbCmd = "gdb -batch -ex \"break " + srcPath.string() + ":10\" "
                         "-ex \"run\" "
                         "-ex \"print a\" "
                         "-ex \"print b\" "
                         "-ex \"print sum\" "
                         "-ex \"print pt\" "
                         "-ex \"bt\" " +
                         exePath.string();
    std::string gdbOut = runShellCapture(gdbCmd, exitCode);
    EXPECT_EQ(exitCode, 0) << gdbOut;
    EXPECT_NE(gdbOut.find("Breakpoint 1"), std::string::npos) << gdbOut;
    EXPECT_NE(gdbOut.find("= 17"), std::string::npos) << gdbOut;
    EXPECT_NE(gdbOut.find("= 25"), std::string::npos) << gdbOut;
    EXPECT_NE(gdbOut.find("= 42"), std::string::npos) << gdbOut;
    EXPECT_NE(gdbOut.find("compute"), std::string::npos) << gdbOut;
    EXPECT_NE(gdbOut.find("main"), std::string::npos) << gdbOut;

    std::filesystem::remove(srcPath);
    std::filesystem::remove(objPath);
    std::filesystem::remove(exePath);
}
