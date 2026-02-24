#include "ParsersTestFixture.h"

TEST_F(ParsersTestFixture, WhileSimple)
{
    std::string input = "while (%a EQ %b) { nop; }";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<WhileParser>());

    std::shared_ptr<WhileAstNode> whileNode;
    EXPECT_TRUE(expectNodeCast<>(whileNode));
    EXPECT_NE(whileNode->getCondition(), nullptr);
    EXPECT_NE(whileNode->getCodeScope(), nullptr);
}

TEST_F(ParsersTestFixture, WhileMissingCondition)
{
    std::string input = "while { nop; }";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<WhileParser>());
}

TEST_F(ParsersTestFixture, WhileMissingScope)
{
    std::string input = "while (%a EQ %b)";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<WhileParser>());
}

TEST_F(ParsersTestFixture, WhileInvalidCondition)
{
    std::string input = "while (%a AS) { nop; }";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<WhileParser>());
}

TEST_F(ParsersTestFixture, WhileEmptyScope)
{
    std::string input = "while (%a EQ %b) {}";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<WhileParser>());

    std::shared_ptr<WhileAstNode> whileNode;
    EXPECT_TRUE(expectNodeCast<>(whileNode));
    EXPECT_NE(whileNode->getCondition(), nullptr);
    EXPECT_NE(whileNode->getCodeScope(), nullptr);
    EXPECT_TRUE(whileNode->getCodeScope()->getExpressions().empty());
}
