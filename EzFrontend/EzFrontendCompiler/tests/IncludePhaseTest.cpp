#include "FrontendCompilerTestFixture.h"

// =============================================================================
//  1. IncludePhase – getName
// =============================================================================

TEST_F(FrontendCompilerTestFixture, IncludePhase_GetName)
{
    IncludePhase phase;
    EXPECT_STREQ(phase.getName(), "IncludePhase");
}

// =============================================================================
//  2. IncludePhase – source without includes
// =============================================================================

TEST_F(FrontendCompilerTestFixture, IncludePhase_NoIncludes_EmptySet)
{
    std::string code = R"(
void F()
{
    nop;
})";
    auto unit = createUnitFromSource(code);
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runUpToParsing(unit.get()));

    IncludePhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));

    std::set<std::string_view> dest;
    phase.moveIncludedFilesToDest(dest);
    EXPECT_TRUE(dest.empty());
}

TEST_F(FrontendCompilerTestFixture, IncludePhase_MultipleModulesNoIncludes)
{
    std::string code = R"(
void A() { nop; }
void B() { nop; }
void C() { nop; }
)";
    auto unit = createUnitFromSource(code);
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runUpToParsing(unit.get()));

    IncludePhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));

    std::set<std::string_view> dest;
    phase.moveIncludedFilesToDest(dest);
    EXPECT_TRUE(dest.empty());
}

// =============================================================================
//  3. IncludePhase – moveIncludedFilesToDest called twice
// =============================================================================

TEST_F(FrontendCompilerTestFixture, IncludePhase_MoveIncludedFiles_SecondCallEmpty)
{
    std::string code = R"(
void F()
{
    nop;
})";
    auto unit = createUnitFromSource(code);
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runUpToParsing(unit.get()));

    IncludePhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));

    std::set<std::string_view> dest1;
    phase.moveIncludedFilesToDest(dest1);

    // Second call after move should yield an empty set
    std::set<std::string_view> dest2;
    phase.moveIncludedFilesToDest(dest2);
    EXPECT_TRUE(dest2.empty());
}

