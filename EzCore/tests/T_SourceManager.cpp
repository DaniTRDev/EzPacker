#include "SourceManager/SourceManager.h"
#include <gtest/gtest.h>
#include <filesystem>

/**
 * Regression for WEI-05: a trailing newline must not synthesize an extra empty line at EOF.
 */
TEST(SourceManagerTest, TrailingNewlineDoesNotCreatePhantomLine)
{
    SourceManager sm(std::filesystem::current_path(), std::pmr::get_default_resource());
    size_t id = sm.addSourceContent("test.src", "a\nb\n");
    ASSERT_EQ(id, 1u);

    // Real second line is still resolvable.
    SourceReference *lastLineRef = sm.createReference(2, 0, id);
    ASSERT_NE(lastLineRef, nullptr);
    const SourceLineRange *lastLine = sm.getReferenceLine(lastLineRef);
    ASSERT_NE(lastLine, nullptr);
    EXPECT_EQ(lastLine->m_lineNumber, 2u);

    // Offset one past the final newline must not resolve to a phantom line.
    SourceReference *eofRef = sm.createReference(4, 0, id);
    ASSERT_NE(eofRef, nullptr);
    EXPECT_EQ(sm.getReferenceLine(eofRef), nullptr);
}

/**
 * Regression for WEI-05: content without a trailing newline keeps its final line.
 */
TEST(SourceManagerTest, FinalLineWithoutTrailingNewline)
{
    SourceManager sm(std::filesystem::current_path(), std::pmr::get_default_resource());
    size_t id = sm.addSourceContent("test.src", "a\nb");
    ASSERT_EQ(id, 1u);

    SourceReference *ref = sm.createReference(2, 0, id);
    ASSERT_NE(ref, nullptr);
    const SourceLineRange *line = sm.getReferenceLine(ref);
    ASSERT_NE(line, nullptr);
    EXPECT_EQ(line->m_lineNumber, 2u);
    EXPECT_EQ(sm.getRawLineContent(ref), "b");
}
