#include "EzLinkerTestSuite.h"

TEST_F(EzLinkerTestSuite, TestLinkerClassification)
{
    EXPECT_EQ(EzLinker::SystemLinkerDetector::classifyLinker("lld-link"), EzLinker::SystemLinkerKind::LldLink);
    EXPECT_EQ(EzLinker::SystemLinkerDetector::classifyLinker("C:\\LLVM\\bin\\lld-link.exe"), EzLinker::SystemLinkerKind::LldLink);
    EXPECT_EQ(EzLinker::SystemLinkerDetector::classifyLinker("link.exe"), EzLinker::SystemLinkerKind::MsvcLink);
    EXPECT_EQ(EzLinker::SystemLinkerDetector::classifyLinker("/usr/bin/ld.lld"), EzLinker::SystemLinkerKind::LldElf);
    EXPECT_EQ(EzLinker::SystemLinkerDetector::classifyLinker("/usr/bin/ld.bfd"), EzLinker::SystemLinkerKind::GnuBfd);
    EXPECT_EQ(EzLinker::SystemLinkerDetector::classifyLinker("/usr/bin/ld.gold"), EzLinker::SystemLinkerKind::GnuGold);
    EXPECT_EQ(EzLinker::SystemLinkerDetector::classifyLinker("clang"), EzLinker::SystemLinkerKind::ClangDriver);
    EXPECT_EQ(EzLinker::SystemLinkerDetector::classifyLinker("gcc"), EzLinker::SystemLinkerKind::GccDriver);
    EXPECT_EQ(EzLinker::SystemLinkerDetector::classifyLinker("unknown_tool"), EzLinker::SystemLinkerKind::Unknown);
}

TEST_F(EzLinkerTestSuite, TestDetectHostLinker)
{
    EzLinker::DetectedLinker detected = EzLinker::SystemLinkerDetector::detect();
    EXPECT_TRUE(detected.isValid());
    EXPECT_FALSE(detected.path.empty());
    EXPECT_NE(detected.kind, EzLinker::SystemLinkerKind::Unknown);
    EXPECT_FALSE(detected.getKindName().empty());
}

TEST_F(EzLinkerTestSuite, TestCustomLinkerOverride)
{
    // Override with an existing detected linker
    EzLinker::DetectedLinker hostLinker = EzLinker::SystemLinkerDetector::detect();
    ASSERT_TRUE(hostLinker.isValid());

    EzLinker::DetectedLinker overridden = EzLinker::SystemLinkerDetector::detect(hostLinker.path.string());
    EXPECT_TRUE(overridden.isValid());
    EXPECT_EQ(overridden.path, hostLinker.path);
    EXPECT_EQ(overridden.kind, hostLinker.kind);
}

TEST_F(EzLinkerTestSuite, TestSearchPathsNotEmpty)
{
    auto paths = EzLinker::SystemLinkerDetector::getSystemSearchPaths();
    EXPECT_FALSE(paths.empty());
}
