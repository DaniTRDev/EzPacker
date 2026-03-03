#include "LoweringPipelineTestFixture.h"

// =============================================================================
//  1. File-based: control_flow_labels.ez
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Labels_FileLoads)
{
    ASSERT_TRUE(runFromFile("control_flow_labels.ez"));
}

TEST_F(LoweringPipelineTestFixture, Labels_ProducesModule)
{
    ASSERT_TRUE(runFromFile("control_flow_labels.ez"));
    ASSERT_NE(getModule(), nullptr);
}

TEST_F(LoweringPipelineTestFixture, Labels_ParamsLinked)
{
    ASSERT_TRUE(runFromFile("control_flow_labels.ez"));
    expectSymbolLinked("x");
    expectSymbolLinked("y");
}

TEST_F(LoweringPipelineTestFixture, Labels_LocalsLinked)
{
    ASSERT_TRUE(runFromFile("control_flow_labels.ez"));
    expectSymbolLinked("counter");
    expectSymbolLinked("acc");
}

TEST_F(LoweringPipelineTestFixture, Labels_LabelSymbolsLinked)
{
    ASSERT_TRUE(runFromFile("control_flow_labels.ez"));

    // Labels themselves become symbols of type Label.
    Symbol *labelInit = resolveInModuleScope("label_init");
    ASSERT_NE(labelInit, nullptr);
    EXPECT_TRUE(isSymbolLinked(labelInit));

    Symbol *labelCompute = resolveInModuleScope("label_compute");
    ASSERT_NE(labelCompute, nullptr);
    EXPECT_TRUE(isSymbolLinked(labelCompute));

    Symbol *labelFinal = resolveInModuleScope("label_final");
    ASSERT_NE(labelFinal, nullptr);
    EXPECT_TRUE(isSymbolLinked(labelFinal));
}

TEST_F(LoweringPipelineTestFixture, Labels_LabelIdsDistinct)
{
    ASSERT_TRUE(runFromFile("control_flow_labels.ez"));

    Symbol *s1 = resolveInModuleScope("label_init");
    Symbol *s2 = resolveInModuleScope("label_compute");
    Symbol *s3 = resolveInModuleScope("label_final");
    ASSERT_NE(s1, nullptr);
    ASSERT_NE(s2, nullptr);
    ASSERT_NE(s3, nullptr);

    MirId id1 = getMirId(s1);
    MirId id2 = getMirId(s2);
    MirId id3 = getMirId(s3);
    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_NE(id1, id3);
}

// =============================================================================
//  2. Inline: single label
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Labels_Inline_SingleEmpty)
{
    std::string code = R"(
void F()
{
    myLabel:
    {
    }
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Labels_Inline_SingleWithBody)
{
    std::string code = R"(
void F()
{
    myLabel:
    {
        nop;
    }
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Labels_Inline_LabelWithInstructions)
{
    std::string code = R"(
void F(i32 %x)
{
    create i32 %y;
    myLabel:
    {
        mov %y, %x;
        add %y, 1;
        nop;
    }
})";
    ASSERT_TRUE(runFromSource(code));
    expectSymbolLinked("y");
}

// =============================================================================
//  3. Inline: multiple labels
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Labels_Inline_TwoLabels)
{
    std::string code = R"(
void F()
{
    lbl1:
    {
        nop;
    }
    lbl2:
    {
        nop;
    }
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Labels_Inline_ThreeLabels)
{
    std::string code = R"(
void F()
{
    first:
    {
        nop;
    }
    second:
    {
        nop;
    }
    third:
    {
        nop;
    }
})";
    ASSERT_TRUE(runFromSource(code));
}

// =============================================================================
//  4. Inline: nested labels
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Labels_Inline_NestedLabel)
{
    std::string code = R"(
void F()
{
    outer:
    {
        inner:
        {
            nop;
        }
    }
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Labels_Inline_DeeplyNested)
{
    std::string code = R"(
void F()
{
    l1:
    {
        l2:
        {
            l3:
            {
                nop;
            }
        }
    }
})";
    ASSERT_TRUE(runFromSource(code));
}

// =============================================================================
//  5. Inline: labels with control flow
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Labels_Inline_WithIfInside)
{
    std::string code = R"(
void F(i32 %a, i32 %b)
{
    myLabel:
    {
        if (%a EQ %b)
        {
            nop;
        }
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Labels_Inline_WithWhileInside)
{
    std::string code = R"(
void F(i32 %i, i32 %n)
{
    loopLabel:
    {
        while (%i LT %n)
        {
            add %i, 1;
        }
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Labels_Inline_MixedContentInLabel)
{
    std::string code = R"(
void F(i32 %a, i32 %b)
{
    create i32 %r;
    work:
    {
        mov %r, %a;
        if (%a GT %b)
        {
            add %r, 1;
        }
        sub %r, 1;
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

// =============================================================================
//  6. Inline: labels interleaved with instructions
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Labels_Inline_InstructionsBetweenLabels)
{
    std::string code = R"(
void F(i32 %x)
{
    create i32 %y;
    mov %y, 0;

    part1:
    {
        add %y, %x;
    }

    add %y, 100;

    part2:
    {
        sub %y, 50;
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectSymbolLinked("y");
}

