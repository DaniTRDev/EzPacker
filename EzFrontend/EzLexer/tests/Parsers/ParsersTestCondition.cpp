#include "ParsersTestFixture.h"

TEST_F(ParsersTestFixture, ConditionEqual)
{
    std::string input = "%a EQ %b";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<ConditionParser>());

    std::shared_ptr<ConditionAstNode> condition;
    EXPECT_TRUE(expectNodeCast<>(condition));
    EXPECT_EQ(condition->getComparisonType(), ConditionComparisonType::Equal);
    EXPECT_EQ(condition->getLeft()->getType(), AstNodeType::Variable);
    EXPECT_EQ(condition->getRight()->getType(), AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, ConditionNotEqual)
{
    std::string input = "%a NE %b";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<ConditionParser>());

    std::shared_ptr<ConditionAstNode> condition;
    EXPECT_TRUE(expectNodeCast<>(condition));
    EXPECT_EQ(condition->getComparisonType(), ConditionComparisonType::NotEqual);
    EXPECT_EQ(condition->getLeft()->getType(), AstNodeType::Variable);
    EXPECT_EQ(condition->getRight()->getType(), AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, ConditionGreaterThan)
{
    std::string input = "%a GT %b";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<ConditionParser>());

    std::shared_ptr<ConditionAstNode> condition;
    EXPECT_TRUE(expectNodeCast<>(condition));
    EXPECT_EQ(condition->getComparisonType(), ConditionComparisonType::GreaterThan);
    EXPECT_EQ(condition->getLeft()->getType(), AstNodeType::Variable);
    EXPECT_EQ(condition->getRight()->getType(), AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, ConditionGreaterThanOrEqual)
{
    std::string input = "%a GE %b";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<ConditionParser>());

    std::shared_ptr<ConditionAstNode> condition;
    EXPECT_TRUE(expectNodeCast<>(condition));
    EXPECT_EQ(condition->getComparisonType(), ConditionComparisonType::GreaterThanOrEqual);
    EXPECT_EQ(condition->getLeft()->getType(), AstNodeType::Variable);
    EXPECT_EQ(condition->getRight()->getType(), AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, ConditionLessThan)
{
    std::string input = "%a LT %b";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<ConditionParser>());

    std::shared_ptr<ConditionAstNode> condition;
    EXPECT_TRUE(expectNodeCast<>(condition));
    EXPECT_EQ(condition->getComparisonType(), ConditionComparisonType::LessThan);
    EXPECT_EQ(condition->getLeft()->getType(), AstNodeType::Variable);
    EXPECT_EQ(condition->getRight()->getType(), AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, ConditionLessThanOrEqual)
{
    std::string input = "%a LE %b";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<ConditionParser>());

    std::shared_ptr<ConditionAstNode> condition;
    EXPECT_TRUE(expectNodeCast<>(condition));
    EXPECT_EQ(condition->getComparisonType(), ConditionComparisonType::LessThanOrEqual);
    EXPECT_EQ(condition->getLeft()->getType(), AstNodeType::Variable);
    EXPECT_EQ(condition->getRight()->getType(), AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, ConditionImmediate)
{
    std::string input = "%a EQ 123";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<ConditionParser>());

    std::shared_ptr<ConditionAstNode> condition;
    EXPECT_TRUE(expectNodeCast<>(condition));
    EXPECT_EQ(condition->getComparisonType(), ConditionComparisonType::Equal);
    EXPECT_EQ(condition->getLeft()->getType(), AstNodeType::Variable);
    EXPECT_EQ(condition->getRight()->getType(), AstNodeType::Immediate);
}

TEST_F(ParsersTestFixture, ConditionInvalid)
{
    std::string input = "%a AS %b";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<ConditionParser>());
}

TEST_F(ParsersTestFixture, ConditionMissingRightOperand)
{
    std::string input = "%a EQ";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<ConditionParser>());
}

TEST_F(ParsersTestFixture, ConditionMissingLeftOperand)
{
    std::string input = "EQ %b";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<ConditionParser>());
}
