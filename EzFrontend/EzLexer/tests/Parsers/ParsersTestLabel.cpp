#include "ParsersTestFixture.h"

// =============================================================================
//  Empty label
// =============================================================================

TEST_F(ParsersTestFixture, Label_Empty)
{
    EXPECT_TRUE(tokenizeAndParse<LabelParser>("myLabel:{}"));
}

// =============================================================================
//  Labels with instructions
// =============================================================================

TEST_F(ParsersTestFixture, Label_1Instruction)
{
    std::string input = R"(
myLabel:
{
    add %myVar, 123;
})";
    EXPECT_TRUE(tokenizeAndParse<LabelParser>(input));
    TEST_LABEL("myLabel", AstNodeType::Instruction);
}

TEST_F(ParsersTestFixture, Label_2Instructions)
{
    std::string input = R"(
myLabel:
{
    add %myVar, 123;
    nop;
})";
    EXPECT_TRUE(tokenizeAndParse<LabelParser>(input));
    TEST_LABEL("myLabel", AstNodeType::Instruction, AstNodeType::Instruction);
}

TEST_F(ParsersTestFixture, Label_ManyInstructions)
{
    std::string input = R"(
myLabel:
{
    add %myVar2, i64 (1234);
    add %myVar3, i32 (%base+0);
    add %myVar4, i16 (%base + 123);
    nop;
})";
    EXPECT_TRUE(tokenizeAndParse<LabelParser>(input));
    TEST_LABEL("myLabel",
               AstNodeType::Instruction,
               AstNodeType::Instruction,
               AstNodeType::Instruction,
               AstNodeType::Instruction);
}

// =============================================================================
//  Nested labels
// =============================================================================

TEST_F(ParsersTestFixture, Label_NestedLabel)
{
    std::string input = R"(
myLabel:
{
    add i8 %myVar, 1;
    myLabel2:
    {
        add %myVar2, i64 (1234);
        add %myVar3, i32 (%base+0);
        add %myVar4, i16 (%base      +       123);
    }
})";
    EXPECT_TRUE(tokenizeAndParse<LabelParser>(input));
    TEST_LABEL("myLabel", AstNodeType::Instruction, AstNodeType::Label);
}

TEST_F(ParsersTestFixture, Label_DeeplyNestedLabel)
{
    std::string input = R"(
outer:
{
    middle:
    {
        inner:
        {
            nop;
        }
    }
})";
    EXPECT_TRUE(tokenizeAndParse<LabelParser>(input));

    Label *outer;
    ASSERT_TRUE(expectNodeCast<>(outer));
    EXPECT_EQ(outer->getLabelName(), "outer");

    auto outerExprs = outer->getCodeScope()->getExpressions();
    ASSERT_EQ(outerExprs->m_numElems, 1);

    // The single child should be "middle" label
    Label *middle = outerExprs->get<Label>(0);
    ASSERT_NE(middle, nullptr);
    EXPECT_EQ(middle->getType(), AstNodeType::Label);
    EXPECT_EQ(middle->getLabelName(), "middle");
}

// =============================================================================
//  Label with if / while inside
// =============================================================================

TEST_F(ParsersTestFixture, Label_ContainsIf)
{
    std::string input = R"(
myLabel:
{
    if (%a EQ %b) { nop; }
})";
    EXPECT_TRUE(tokenizeAndParse<LabelParser>(input));
    TEST_LABEL("myLabel", AstNodeType::If);
}

TEST_F(ParsersTestFixture, Label_ContainsWhile)
{
    std::string input = R"(
myLabel:
{
    while (%x LT %y) { nop; }
})";
    EXPECT_TRUE(tokenizeAndParse<LabelParser>(input));
    TEST_LABEL("myLabel", AstNodeType::While);
}

TEST_F(ParsersTestFixture, Label_MixedContent)
{
    std::string input = R"(
myLabel:
{
    nop;
    if (%a EQ %b) { nop; }
    nop;
    while (%c LT %d) { nop; }
})";
    EXPECT_TRUE(tokenizeAndParse<LabelParser>(input));
    TEST_LABEL("myLabel",
               AstNodeType::Instruction,
               AstNodeType::If,
               AstNodeType::Instruction,
               AstNodeType::While);
}

// =============================================================================
//  Label error cases
// =============================================================================

TEST_F(ParsersTestFixture, Label_InvalidInstr)
{
    std::string input = R"(
myLabel:
{
    ;
})";
    EXPECT_FALSE(tokenizeAndParse<LabelParser>(input));
}

TEST_F(ParsersTestFixture, Label_MissingColon)
{
    EXPECT_FALSE(tokenizeAndParse<LabelParser>("myLabel {}"));
}

TEST_F(ParsersTestFixture, Label_MissingLeftBrace)
{
    EXPECT_FALSE(tokenizeAndParse<LabelParser>("myLabel: "));
}

TEST_F(ParsersTestFixture, Label_MissingRightBrace)
{
    EXPECT_FALSE(tokenizeAndParse<LabelParser>("myLabel: { nop; "));
}
