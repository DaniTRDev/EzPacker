#include "ParsersTestFixture.h"

// =============================================================================
//  Basic while
// =============================================================================

TEST_F(ParsersTestFixture, While_Simple)
{
    EXPECT_TRUE(tokenizeAndParse<WhileParser>("while (%a EQ %b) { break; }"));
    TEST_WHILE(1);
}

TEST_F(ParsersTestFixture, While_MultipleStatements)
{
    EXPECT_TRUE(tokenizeAndParse<WhileParser>("while (%x LT %y) { nop; continue; }"));
    TEST_WHILE(3);
}

TEST_F(ParsersTestFixture, While_EmptyBody)
{
    EXPECT_TRUE(tokenizeAndParse<WhileParser>("while (%a EQ %b) {}"));
    TEST_WHILE(0);
}

// =============================================================================
//  All condition operators in while
// =============================================================================

TEST_F(ParsersTestFixture, While_ConditionEQ)
{
    EXPECT_TRUE(tokenizeAndParse<WhileParser>("while (%a EQ %b) { nop; }"));
    WhileAstNode *w;
    ASSERT_TRUE(expectNodeCast<>(w));
    EXPECT_EQ(w->getCondition()->getComparisonType(), ConditionComparisonType::Equal);
}

TEST_F(ParsersTestFixture, While_ConditionNE)
{
    EXPECT_TRUE(tokenizeAndParse<WhileParser>("while (%a NE %b) { nop; }"));
    WhileAstNode *w;
    ASSERT_TRUE(expectNodeCast<>(w));
    EXPECT_EQ(w->getCondition()->getComparisonType(), ConditionComparisonType::NotEqual);
}

TEST_F(ParsersTestFixture, While_ConditionGT)
{
    EXPECT_TRUE(tokenizeAndParse<WhileParser>("while (%a GT %b) { nop; }"));
    WhileAstNode *w;
    ASSERT_TRUE(expectNodeCast<>(w));
    EXPECT_EQ(w->getCondition()->getComparisonType(), ConditionComparisonType::GreaterThan);
}

TEST_F(ParsersTestFixture, While_ConditionGE)
{
    EXPECT_TRUE(tokenizeAndParse<WhileParser>("while (%a GE %b) { nop; }"));
    WhileAstNode *w;
    ASSERT_TRUE(expectNodeCast<>(w));
    EXPECT_EQ(w->getCondition()->getComparisonType(), ConditionComparisonType::GreaterThanOrEqual);
}

TEST_F(ParsersTestFixture, While_ConditionLT)
{
    EXPECT_TRUE(tokenizeAndParse<WhileParser>("while (%a LT %b) { nop; }"));
    WhileAstNode *w;
    ASSERT_TRUE(expectNodeCast<>(w));
    EXPECT_EQ(w->getCondition()->getComparisonType(), ConditionComparisonType::LessThan);
}

TEST_F(ParsersTestFixture, While_ConditionLE)
{
    EXPECT_TRUE(tokenizeAndParse<WhileParser>("while (%a LE %b) { nop; }"));
    WhileAstNode *w;
    ASSERT_TRUE(expectNodeCast<>(w));
    EXPECT_EQ(w->getCondition()->getComparisonType(), ConditionComparisonType::LessThanOrEqual);
}

// =============================================================================
//  While with various body content
// =============================================================================

TEST_F(ParsersTestFixture, While_BodyWithIfInside)
{
    std::string input = "while (%i LT %n) { if (%i EQ %j) { nop; } }";
    EXPECT_TRUE(tokenizeAndParse<WhileParser>(input));
    TEST_WHILE(1);

    WhileAstNode *w;
    ASSERT_TRUE(expectNodeCast<>(w));
    auto exprs = w->getCodeScope()->getExpressions();
    AstNode *child = (AstNode *)exprs->m_head->m_object;
    EXPECT_EQ(child->getType(), AstNodeType::If);
}

TEST_F(ParsersTestFixture, While_BodyWithLabel)
{
    std::string input = "while (%a LT %b) { myLabel: { nop; } }";
    EXPECT_TRUE(tokenizeAndParse<WhileParser>(input));
    TEST_WHILE(1);

    WhileAstNode *w;
    ASSERT_TRUE(expectNodeCast<>(w));
    auto exprs = w->getCodeScope()->getExpressions();
    AstNode *child = (AstNode *)exprs->m_head->m_object;
    EXPECT_EQ(child->getType(), AstNodeType::Label);
}

TEST_F(ParsersTestFixture, While_BodyWithCreateAndArith)
{
    std::string input = "while (%i LT %n) { create i32 %tmp; add %i, 1; }";
    EXPECT_TRUE(tokenizeAndParse<WhileParser>(input));
    TEST_WHILE(2);
}

// =============================================================================
//  While error cases
// =============================================================================

TEST_F(ParsersTestFixture, While_MissingCondition) { EXPECT_FALSE(tokenizeAndParse<WhileParser>("while { nop; }")); }

TEST_F(ParsersTestFixture, While_MissingBody) { EXPECT_FALSE(tokenizeAndParse<WhileParser>("while (%a EQ %b)")); }

TEST_F(ParsersTestFixture, While_InvalidCondition)
{
    EXPECT_FALSE(tokenizeAndParse<WhileParser>("while (%a AS) { nop; }"));
}

TEST_F(ParsersTestFixture, While_MissingLeftParen)
{
    EXPECT_FALSE(tokenizeAndParse<WhileParser>("while %a EQ %b) { nop; }"));
}

TEST_F(ParsersTestFixture, While_MissingRightParen)
{
    EXPECT_FALSE(tokenizeAndParse<WhileParser>("while (%a EQ %b { nop; }"));
}
