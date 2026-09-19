#include "EzCompilerTestSuite.h"

using namespace EzCompiler;

// Verifies parsing 4-component, 2-component, and host triples into arch/vendor/sys/abi with derived predicates.
TEST_F(EzCompilerTestSuite, TestTargetTripleParsing)
{
    // 4 components
    TargetTriple t1 = TargetTriple::parse("x86_64-unknown-linux-gnu");
    EXPECT_EQ(t1.getArch(), "x86_64");
    EXPECT_EQ(t1.getVendor(), "unknown");
    EXPECT_EQ(t1.getSys(), "linux");
    EXPECT_EQ(t1.getAbi(), "gnu");
    EXPECT_TRUE(t1.isX86_64());
    EXPECT_TRUE(t1.isLinux());
    EXPECT_TRUE(t1.isElf());
    EXPECT_FALSE(t1.isWindows());
    EXPECT_FALSE(t1.isCoff());

    // Windows MSVC
    TargetTriple t2 = TargetTriple::parse("x86_64-pc-windows-msvc");
    EXPECT_EQ(t2.getArch(), "x86_64");
    EXPECT_EQ(t2.getVendor(), "pc");
    EXPECT_EQ(t2.getSys(), "windows");
    EXPECT_EQ(t2.getAbi(), "msvc");
    EXPECT_TRUE(t2.isX86_64());
    EXPECT_TRUE(t2.isWindows());
    EXPECT_TRUE(t2.isCoff());
    EXPECT_FALSE(t2.isLinux());

    // 2 components
    TargetTriple tElf = TargetTriple::parse("x86_64-elf");
    EXPECT_TRUE(tElf.isElf());
    EXPECT_FALSE(tElf.isWindows());

    TargetTriple tCoff = TargetTriple::parse("x86_64-coff");
    EXPECT_TRUE(tCoff.isCoff());
    EXPECT_TRUE(tCoff.isWindows());

    // Host triple
    TargetTriple host = TargetTriple::getHostTriple();
    EXPECT_FALSE(host.getArch().empty());
    EXPECT_TRUE(host.isX86_64());
}

// Verifies resolving Linux and Windows triples selects the expected target descriptor, calling convention, and binary
// descriptor.
TEST_F(EzCompilerTestSuite, TestTargetResolverResolution)
{
    CommandLineOptions optLinux;
    optLinux.target = TargetTriple::parse("x86_64-unknown-linux-gnu");

    DriverContext ctxLinux(optLinux);
    EXPECT_TRUE(ctxLinux.initialize());
    ASSERT_NE(ctxLinux.getTargetDesc(), nullptr);
    ASSERT_NE(ctxLinux.getCallingConv(), nullptr);
    ASSERT_NE(ctxLinux.getBinaryDesc(), nullptr);

    EXPECT_STREQ(ctxLinux.getTargetDesc()->getName(), "x86_64");
    EXPECT_STREQ(ctxLinux.getCallingConv()->getName(), "SysV_AMD64");
    EXPECT_STREQ(ctxLinux.getBinaryDesc()->getName(), "x86_64-elf");

    CommandLineOptions optWin;
    optWin.target = TargetTriple::parse("x86_64-pc-windows-msvc");

    DriverContext ctxWin(optWin);
    EXPECT_TRUE(ctxWin.initialize());
    ASSERT_NE(ctxWin.getTargetDesc(), nullptr);
    ASSERT_NE(ctxWin.getCallingConv(), nullptr);
    ASSERT_NE(ctxWin.getBinaryDesc(), nullptr);

    EXPECT_STREQ(ctxWin.getTargetDesc()->getName(), "x86_64");
    EXPECT_STREQ(ctxWin.getCallingConv()->getName(), "Win64");
    EXPECT_STREQ(ctxWin.getBinaryDesc()->getName(), "x86_64-coff");
}
