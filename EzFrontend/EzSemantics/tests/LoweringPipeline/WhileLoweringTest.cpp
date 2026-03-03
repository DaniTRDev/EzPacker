#include "LoweringPipelineTestFixture.h"

// =============================================================================
//  1. File-based: control_flow_while.ez
// =============================================================================

TEST_F(LoweringPipelineTestFixture, While_FileLoads)
{
    ASSERT_TRUE(runFromFile("control_flow_while.ez"));
}

TEST_F(LoweringPipelineTestFixture, While_ProducesModule)
{
    ASSERT_TRUE(runFromFile("control_flow_while.ez"));
    ASSERT_NE(getModule(), nullptr);
}

TEST_F(LoweringPipelineTestFixture, While_ParamsLinked)
{
    ASSERT_TRUE(runFromFile("control_flow_while.ez"));
    expectSymbolLinked("n");
    expectSymbolLinked("m");
}

TEST_F(LoweringPipelineTestFixture, While_LocalsLinked)
{
    ASSERT_TRUE(runFromFile("control_flow_while.ez"));
    expectSymbolLinked("i");
    expectSymbolLinked("j");
    expectSymbolLinked("sum");
}

TEST_F(LoweringPipelineTestFixture, While_AllDistinct)
{
    ASSERT_TRUE(runFromFile("control_flow_while.ez"));
    expectDistinctMirIds("n", "m");
    expectDistinctMirIds("i", "j");
    expectDistinctMirIds("i", "sum");
    expectDistinctMirIds("j", "sum");
}

// =============================================================================
//  2. Inline: simple while
// =============================================================================

TEST_F(LoweringPipelineTestFixture, While_Inline_Simple)
{
    std::string code = R"(
void F(i32 %i, i32 %n)
{
    while (%i LT %n)
    {
        add %i, 1;
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, While_Inline_CondEQ)
{
    std::string code = R"(
void F(i32 %i, i32 %n)
{
    while (%i EQ %n)
    {
        nop;
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, While_Inline_CondNE)
{
    std::string code = R"(
void F(i32 %i, i32 %n)
{
    while (%i NE %n)
    {
        add %i, 1;
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, While_Inline_CondGT)
{
    std::string code = R"(
void F(i32 %i, i32 %n)
{
    while (%i GT %n)
    {
        sub %i, 1;
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, While_Inline_CondGE)
{
    std::string code = R"(
void F(i32 %i, i32 %n)
{
    while (%i GE %n)
    {
        sub %i, 1;
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, While_Inline_CondLE)
{
    std::string code = R"(
void F(i32 %i, i32 %n)
{
    while (%i LE %n)
    {
        add %i, 1;
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

// =============================================================================
//  3. Inline: nested while
// =============================================================================

TEST_F(LoweringPipelineTestFixture, While_Inline_Nested)
{
    std::string code = R"(
void F(i32 %i, i32 %j, i32 %n)
{
    while (%i LT %n)
    {
        while (%j LT %n)
        {
            add %j, 1;
        }
        add %i, 1;
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, While_Inline_TripleNested)
{
    std::string code = R"(
void F(i32 %a, i32 %b, i32 %c, i32 %n)
{
    while (%a LT %n)
    {
        while (%b LT %n)
        {
            while (%c LT %n)
            {
                add %c, 1;
            }
            add %b, 1;
        }
        add %a, 1;
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

// =============================================================================
//  4. Inline: while with if inside
// =============================================================================

TEST_F(LoweringPipelineTestFixture, While_Inline_WithIfInside)
{
    std::string code = R"(
void F(i32 %i, i32 %n)
{
    create i32 %evens;
    mov %evens, 0;

    while (%i LT %n)
    {
        create i32 %tmp;
        mov %tmp, %i;
        and %tmp, 1;
        if (%tmp EQ %i)
        {
            add %evens, 1;
        }
        add %i, 1;
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, While_Inline_WithIfElseInside)
{
    std::string code = R"(
void F(i32 %x, i32 %limit)
{
    create i32 %acc;
    mov %acc, 0;

    while (%x LT %limit)
    {
        if (%x GT %acc)
        {
            mov %acc, %x;
        }
        else
        {
            add %acc, 1;
        }
        add %x, 1;
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

// =============================================================================
//  5. Inline: empty body / edge cases
// =============================================================================

TEST_F(LoweringPipelineTestFixture, While_Inline_EmptyBody)
{
    std::string code = R"(
void F(i32 %a, i32 %b)
{
    while (%a LT %b)
    {
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, While_Inline_BodyWithCreate)
{
    std::string code = R"(
void F(i32 %i, i32 %n)
{
    while (%i LT %n)
    {
        create i32 %tmp;
        mov %tmp, %i;
        add %i, 1;
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, While_Inline_FollowedByInstructions)
{
    std::string code = R"(
void F(i32 %i, i32 %n)
{
    create i32 %result;
    mov %result, 0;

    while (%i LT %n)
    {
        add %result, %i;
        add %i, 1;
    }

    add %result, 100;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectSymbolLinked("result");
}

TEST_F(LoweringPipelineTestFixture, While_Inline_MultipleWhiles)
{
    std::string code = R"(
void F(i32 %i, i32 %j, i32 %n)
{
    while (%i LT %n)
    {
        add %i, 1;
    }
    while (%j LT %n)
    {
        add %j, 1;
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

