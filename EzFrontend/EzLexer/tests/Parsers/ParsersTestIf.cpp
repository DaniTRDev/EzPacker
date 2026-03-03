#include "ParsersTestFixture.h"

// =============================================================================
//  Simple if (no else)
// =============================================================================

TEST_F(ParsersTestFixture, If_Simple)
{
    EXPECT_TRUE(tokenizeAndParse<IfParser>("if (%a EQ %b) { nop; }"));
    TEST_IF(true, false, AstNodeType::Invalid);
}

TEST_F(ParsersTestFixture, If_MultipleStatements)
{
    EXPECT_TRUE(tokenizeAndParse<IfParser>("if (%a NE %b) { nop; nop; nop; }"));
    TEST_IF(true, false, AstNodeType::Invalid);

    IfAstNode *ifNode;
    ASSERT_TRUE(expectNodeCast<>(ifNode));
    EXPECT_EQ(ifNode->getTrueScope()->getExpressions()->m_numElems, 3);
}

TEST_F(ParsersTestFixture, If_EmptyBody)
{
    EXPECT_TRUE(tokenizeAndParse<IfParser>("if (%a EQ %b) {}"));
    TEST_IF(true, false, AstNodeType::Invalid);

    IfAstNode *ifNode;
    ASSERT_TRUE(expectNodeCast<>(ifNode));
    EXPECT_EQ(ifNode->getTrueScope()->getExpressions()->m_numElems, 0);
}

// =============================================================================
//  If-else
// =============================================================================

TEST_F(ParsersTestFixture, If_Else)
{
    EXPECT_TRUE(tokenizeAndParse<IfParser>("if (%a EQ %b) { nop; } else { nop; }"));
    TEST_IF(true, true, AstNodeType::CodeScope);
}

TEST_F(ParsersTestFixture, If_ElseEmptyBodies)
{
    EXPECT_TRUE(tokenizeAndParse<IfParser>("if (%a EQ %b) {} else {}"));
    TEST_IF(true, true, AstNodeType::CodeScope);
}

// =============================================================================
//  If-elseif chains
// =============================================================================

TEST_F(ParsersTestFixture, If_ElseIf)
{
    EXPECT_TRUE(tokenizeAndParse<IfParser>("if (%a EQ %b) { nop; } else if (%a GT %b) { nop; }"));
    TEST_IF(true, true, AstNodeType::If);
}

TEST_F(ParsersTestFixture, If_ElseIfElse)
{
    EXPECT_TRUE(tokenizeAndParse<IfParser>(
            "if (%a EQ %b) { nop; } else if (%a GT %b) { nop; } else { nop; }"));
    TEST_IF(true, true, AstNodeType::If);

    // Verify the chain
    IfAstNode *ifNode;
    ASSERT_TRUE(expectNodeCast<>(ifNode));

    IfAstNode *elseIfNode = dynamic_cast<IfAstNode *>(ifNode->getFalseScope());
    ASSERT_NE(elseIfNode, nullptr);
    EXPECT_NE(elseIfNode->getCondition(), nullptr);
    EXPECT_NE(elseIfNode->getTrueScope(), nullptr);
    EXPECT_NE(elseIfNode->getFalseScope(), nullptr);
    EXPECT_EQ(elseIfNode->getFalseScope()->getType(), AstNodeType::CodeScope);
}

TEST_F(ParsersTestFixture, If_LongElseIfChain)
{
    std::string input =
            "if (%a EQ %b) { nop; } "
            "else if (%a GT %b) { nop; } "
            "else if (%a LT %b) { nop; } "
            "else { nop; }";
    EXPECT_TRUE(tokenizeAndParse<IfParser>(input));
    TEST_IF(true, true, AstNodeType::If);
}

// =============================================================================
//  All condition types in if
// =============================================================================

TEST_F(ParsersTestFixture, If_ConditionEQ)
{
    EXPECT_TRUE(tokenizeAndParse<IfParser>("if (%x EQ %y) { nop; }"));
    IfAstNode *ifNode;
    ASSERT_TRUE(expectNodeCast<>(ifNode));
    EXPECT_EQ(dynamic_cast<ConditionAstNode *>(ifNode->getCondition())->getComparisonType(),
              ConditionComparisonType::Equal);
}

TEST_F(ParsersTestFixture, If_ConditionNE)
{
    EXPECT_TRUE(tokenizeAndParse<IfParser>("if (%x NE %y) { nop; }"));
    IfAstNode *ifNode;
    ASSERT_TRUE(expectNodeCast<>(ifNode));
    EXPECT_EQ(dynamic_cast<ConditionAstNode *>(ifNode->getCondition())->getComparisonType(),
              ConditionComparisonType::NotEqual);
}

TEST_F(ParsersTestFixture, If_ConditionGT)
{
    EXPECT_TRUE(tokenizeAndParse<IfParser>("if (%x GT %y) { nop; }"));
    IfAstNode *ifNode;
    ASSERT_TRUE(expectNodeCast<>(ifNode));
    EXPECT_EQ(dynamic_cast<ConditionAstNode *>(ifNode->getCondition())->getComparisonType(),
              ConditionComparisonType::GreaterThan);
}

TEST_F(ParsersTestFixture, If_ConditionGE)
{
    EXPECT_TRUE(tokenizeAndParse<IfParser>("if (%x GE %y) { nop; }"));
    IfAstNode *ifNode;
    ASSERT_TRUE(expectNodeCast<>(ifNode));
    EXPECT_EQ(dynamic_cast<ConditionAstNode *>(ifNode->getCondition())->getComparisonType(),
              ConditionComparisonType::GreaterThanOrEqual);
}

TEST_F(ParsersTestFixture, If_ConditionLT)
{
    EXPECT_TRUE(tokenizeAndParse<IfParser>("if (%x LT %y) { nop; }"));
    IfAstNode *ifNode;
    ASSERT_TRUE(expectNodeCast<>(ifNode));
    EXPECT_EQ(dynamic_cast<ConditionAstNode *>(ifNode->getCondition())->getComparisonType(),
              ConditionComparisonType::LessThan);
}

TEST_F(ParsersTestFixture, If_ConditionLE)
{
    EXPECT_TRUE(tokenizeAndParse<IfParser>("if (%x LE %y) { nop; }"));
    IfAstNode *ifNode;
    ASSERT_TRUE(expectNodeCast<>(ifNode));
    EXPECT_EQ(dynamic_cast<ConditionAstNode *>(ifNode->getCondition())->getComparisonType(),
              ConditionComparisonType::LessThanOrEqual);
}

// =============================================================================
//  If error cases
// =============================================================================

TEST_F(ParsersTestFixture, If_MissingCondition)
{
    EXPECT_FALSE(tokenizeAndParse<IfParser>("if { nop; }"));
}

TEST_F(ParsersTestFixture, If_MissingBody)
{
    EXPECT_FALSE(tokenizeAndParse<IfParser>("if (%a EQ %b)"));
}

TEST_F(ParsersTestFixture, If_InvalidCondition)
{
    EXPECT_FALSE(tokenizeAndParse<IfParser>("if (%a ASD) { nop; }"));
}

TEST_F(ParsersTestFixture, If_ElseMissingBody)
{
    EXPECT_FALSE(tokenizeAndParse<IfParser>("if (%a EQ %b) { nop; } else"));
}

TEST_F(ParsersTestFixture, If_ElseIfMissingCondition)
{
    EXPECT_FALSE(tokenizeAndParse<IfParser>("if (%a EQ %b) { nop; } else if { nop; }"));
}

TEST_F(ParsersTestFixture, If_MissingLeftParen)
{
    EXPECT_FALSE(tokenizeAndParse<IfParser>("if %a EQ %b) { nop; }"));
}

TEST_F(ParsersTestFixture, If_MissingRightParen)
{
    EXPECT_FALSE(tokenizeAndParse<IfParser>("if (%a EQ %b { nop; }"));
}
