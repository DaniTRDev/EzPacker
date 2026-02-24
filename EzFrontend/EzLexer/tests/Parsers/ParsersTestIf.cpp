#include "ParsersTestFixture.h"

TEST_F(ParsersTestFixture, IfSimple)
{
    std::string input = "if (%a EQ %b) { nop; }";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<IfParser>());

    std::shared_ptr<IfAstNode> ifNode;
    EXPECT_TRUE(expectNodeCast<>(ifNode));
    EXPECT_NE(ifNode->getCondition(), nullptr);
    EXPECT_NE(ifNode->getTrueScope(), nullptr);
    EXPECT_EQ(ifNode->getFalseScope(), nullptr);
}

TEST_F(ParsersTestFixture, IfElse)
{
    std::string input = "if (%a EQ %b) { nop; } else { nop; }";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<IfParser>());

    std::shared_ptr<IfAstNode> ifNode;
    EXPECT_TRUE(expectNodeCast<>(ifNode));
    EXPECT_NE(ifNode->getCondition(), nullptr);
    EXPECT_NE(ifNode->getTrueScope(), nullptr);
    EXPECT_NE(ifNode->getFalseScope(), nullptr);
    EXPECT_EQ(ifNode->getFalseScope()->getType(), AstNodeType::CodeScope);
}

TEST_F(ParsersTestFixture, IfElseIf)
{
    std::string input = "if (%a EQ %b) { nop; } else if (%a GT %b) { nop; }";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<IfParser>());

    std::shared_ptr<IfAstNode> ifNode;
    EXPECT_TRUE(expectNodeCast<>(ifNode));
    EXPECT_NE(ifNode->getCondition(), nullptr);
    EXPECT_NE(ifNode->getTrueScope(), nullptr);
    EXPECT_NE(ifNode->getFalseScope(), nullptr);
    EXPECT_EQ(ifNode->getFalseScope()->getType(), AstNodeType::If);
}

TEST_F(ParsersTestFixture, IfElseIfElse)
{
    std::string input = "if (%a EQ %b) { nop; } else if (%a GT %b) { nop; } else { nop; }";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<IfParser>());

    std::shared_ptr<IfAstNode> ifNode;
    EXPECT_TRUE(expectNodeCast<>(ifNode));
    EXPECT_NE(ifNode->getCondition(), nullptr);
    EXPECT_NE(ifNode->getTrueScope(), nullptr);
    EXPECT_NE(ifNode->getFalseScope(), nullptr);
    EXPECT_EQ(ifNode->getFalseScope()->getType(), AstNodeType::If);

    std::shared_ptr<IfAstNode> elseIfNode = std::dynamic_pointer_cast<IfAstNode>(ifNode->getFalseScope());
    EXPECT_NE(elseIfNode->getCondition(), nullptr);
    EXPECT_NE(elseIfNode->getTrueScope(), nullptr);
    EXPECT_NE(elseIfNode->getFalseScope(), nullptr);
    EXPECT_EQ(elseIfNode->getFalseScope()->getType(), AstNodeType::CodeScope);
}

TEST_F(ParsersTestFixture, IfMissingCondition)
{
    std::string input = "if { nop; }";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<IfParser>());
}

TEST_F(ParsersTestFixture, IfMissingScope)
{
    std::string input = "if (%a EQ %b)";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<IfParser>());
}

TEST_F(ParsersTestFixture, IfInvalidCondition)
{
    std::string input = "if (%a ASD) { nop; }";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<IfParser>());
}

TEST_F(ParsersTestFixture, IfElseMissingScope)
{
    std::string input = "if (%a EQ %b) { nop; } else";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<IfParser>());
}

TEST_F(ParsersTestFixture, IfElseIfMissingCondition)
{
    std::string input = "if (%a EQ %b) { nop; } else if { nop; }";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<IfParser>());
}
