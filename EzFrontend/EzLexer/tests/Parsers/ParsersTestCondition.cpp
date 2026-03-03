#include "ParsersTestFixture.h"

// =============================================================================
//  All comparison operators
// =============================================================================

TEST_F(ParsersTestFixture, Condition_Equal)
{
    EXPECT_TRUE(tokenizeAndParse<ConditionParser>("%a EQ %b"));
    TEST_CONDITION(ConditionComparisonType::Equal, AstNodeType::Variable, AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, Condition_NotEqual)
{
    EXPECT_TRUE(tokenizeAndParse<ConditionParser>("%a NE %b"));
    TEST_CONDITION(ConditionComparisonType::NotEqual, AstNodeType::Variable, AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, Condition_GreaterThan)
{
    EXPECT_TRUE(tokenizeAndParse<ConditionParser>("%a GT %b"));
    TEST_CONDITION(ConditionComparisonType::GreaterThan, AstNodeType::Variable, AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, Condition_GreaterThanOrEqual)
{
    EXPECT_TRUE(tokenizeAndParse<ConditionParser>("%a GE %b"));
    TEST_CONDITION(ConditionComparisonType::GreaterThanOrEqual, AstNodeType::Variable, AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, Condition_LessThan)
{
    EXPECT_TRUE(tokenizeAndParse<ConditionParser>("%a LT %b"));
    TEST_CONDITION(ConditionComparisonType::LessThan, AstNodeType::Variable, AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, Condition_LessThanOrEqual)
{
    EXPECT_TRUE(tokenizeAndParse<ConditionParser>("%a LE %b"));
    TEST_CONDITION(ConditionComparisonType::LessThanOrEqual, AstNodeType::Variable, AstNodeType::Variable);
}

// =============================================================================
//  Mixed operand types
// =============================================================================

TEST_F(ParsersTestFixture, Condition_VarVsImmediate)
{
    EXPECT_TRUE(tokenizeAndParse<ConditionParser>("%a EQ 123"));
    TEST_CONDITION(ConditionComparisonType::Equal, AstNodeType::Variable, AstNodeType::Immediate);
}

TEST_F(ParsersTestFixture, Condition_VarVsHex)
{
    EXPECT_TRUE(tokenizeAndParse<ConditionParser>("%a LT 0xFF"));
    TEST_CONDITION(ConditionComparisonType::LessThan, AstNodeType::Variable, AstNodeType::Immediate);
}

// =============================================================================
//  Error cases
// =============================================================================

TEST_F(ParsersTestFixture, Condition_InvalidOperator)
{
    EXPECT_FALSE(tokenizeAndParse<ConditionParser>("%a AS %b"));
}

TEST_F(ParsersTestFixture, Condition_MissingRight)
{
    EXPECT_FALSE(tokenizeAndParse<ConditionParser>("%a EQ"));
}

TEST_F(ParsersTestFixture, Condition_MissingLeft)
{
    EXPECT_FALSE(tokenizeAndParse<ConditionParser>("EQ %b"));
}

TEST_F(ParsersTestFixture, Condition_MissingOperator)
{
    EXPECT_FALSE(tokenizeAndParse<ConditionParser>("%a %b"));
}
