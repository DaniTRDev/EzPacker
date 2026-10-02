#include "EzLinkerTestSuite.h"
#include <algorithm>

TEST_F(EzLinkerTestSuite, TestMsvcArgumentGeneration)
{
    EzLinker::EzLinkerDriver driver;
    EzLinker::DetectedLinker msvcLinker;
    msvcLinker.kind = EzLinker::SystemLinkerKind::MsvcLink;
    msvcLinker.path = "link.exe";

    EzLinker::LinkerOptions opts;
    opts.inputFiles = { "main.obj", "utils.obj" };
    opts.outputFile = "test.exe";
    opts.libSearchPaths = { "C:\\libs" };
    opts.libraries = { "mylib" };
    opts.subsystem = "console";
    opts.entryPoint = "mainCRTStartup";

    auto args = driver.buildLinkerArguments(opts, msvcLinker);

    auto hasArg = [&](const std::string &expected) {
        return std::find(args.begin(), args.end(), expected) != args.end();
    };

    EXPECT_TRUE(hasArg("/nologo"));
    EXPECT_TRUE(hasArg("/out:test.exe"));
    EXPECT_TRUE(hasArg("main.obj"));
    EXPECT_TRUE(hasArg("utils.obj"));
    EXPECT_TRUE(hasArg("/libpath:C:\\libs"));
    EXPECT_TRUE(hasArg("mylib.lib"));
    EXPECT_TRUE(hasArg("msvcrt.lib"));
    EXPECT_TRUE(hasArg("kernel32.lib"));
    EXPECT_TRUE(hasArg("/subsystem:console"));
    EXPECT_TRUE(hasArg("/entry:mainCRTStartup"));
}

TEST_F(EzLinkerTestSuite, TestElfArgumentGeneration)
{
    EzLinker::EzLinkerDriver driver;
    EzLinker::DetectedLinker elfLinker;
    elfLinker.kind = EzLinker::SystemLinkerKind::LldElf;
    elfLinker.path = "/usr/bin/ld.lld";

    EzLinker::LinkerOptions opts;
    opts.inputFiles = { "main.o" };
    opts.outputFile = "test.out";
    opts.libSearchPaths = { "/usr/local/lib" };
    opts.libraries = { "m" };
    opts.isShared = true;

    auto args = driver.buildLinkerArguments(opts, elfLinker);

    auto hasArg = [&](const std::string &expected) {
        return std::find(args.begin(), args.end(), expected) != args.end();
    };

    EXPECT_TRUE(hasArg("-o"));
    EXPECT_TRUE(hasArg("test.out"));
    EXPECT_TRUE(hasArg("main.o"));
    EXPECT_TRUE(hasArg("-L/usr/local/lib"));
    EXPECT_TRUE(hasArg("-lm"));
    EXPECT_TRUE(hasArg("-lc"));
    EXPECT_TRUE(hasArg("-lgcc"));
    EXPECT_TRUE(hasArg("-shared"));
}

TEST_F(EzLinkerTestSuite, TestDryRunExecution)
{
    EzLinker::EzLinkerDriver driver;
    EzLinker::LinkerOptions opts;
    opts.inputFiles = { "test.obj" };
    opts.outputFile = "out.exe";
    opts.dryRun = true;

    EzLinker::LinkResult result = driver.link(opts);
    EXPECT_EQ(result.exitCode, 0);
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.commandLine.empty());
}

TEST_F(EzLinkerTestSuite, TestNoDefaultLibsFlag)
{
    EzLinker::EzLinkerDriver driver;
    EzLinker::DetectedLinker msvcLinker;
    msvcLinker.kind = EzLinker::SystemLinkerKind::MsvcLink;
    msvcLinker.path = "link.exe";

    EzLinker::LinkerOptions opts;
    opts.inputFiles = { "test.obj" };
    opts.noDefaultLibs = true;
    opts.noCompilerRt = true;
    opts.noExceptionRuntime = true;

    auto args = driver.buildLinkerArguments(opts, msvcLinker);

    auto hasArg = [&](const std::string &expected) {
        return std::find(args.begin(), args.end(), expected) != args.end();
    };

    EXPECT_FALSE(hasArg("msvcrt.lib"));
    EXPECT_FALSE(hasArg("kernel32.lib"));
    EXPECT_FALSE(hasArg("EzExceptionRuntime.lib"));
}
