#include "BasicParserTest.h"

TEST(MemoryParserTest, TestMemoryReferenceBase)
{
    auto result = BasicParserTest::testRule(false, grammar::base(), "(%rcx)");
    EXPECT_EQ(result->getChildren().size(), 1);
    EXPECT_EQ(result->getChildren()[0]->getType(), AstType::Memory);
    EXPECT_EQ(result->getChildren()[0]->getChildren()[0]->getType(), AstType::Register);
}

TEST(MemoryParserTest, TestMemoryReferenceBaseDispl)
{
    auto result = BasicParserTest::testRule(false, grammar::baseDispl(), "(%rcx, 4)");
    EXPECT_EQ(result->getChildren()[0]->getType(), AstType::Memory);
    EXPECT_EQ(result->getChildren()[0]->getChildren().size(), 2);
    EXPECT_EQ(result->getChildren()[0]->getChildren()[0]->getType(), AstType::Register);
    EXPECT_EQ(result->getChildren()[0]->getChildren()[1]->getType(), AstType::Value);
}

TEST(MemoryParserTest, TestMemoryReferenceBaseIndexScaleDispl)
{
    auto result = BasicParserTest::testRule(false, grammar::baseIndexScaleDisplacement(), "(%rcx, %rbx, 1, 4)");
    EXPECT_EQ(result->getChildren()[0]->getType(), AstType::Memory);
    EXPECT_EQ(result->getChildren()[0]->getChildren().size(), 4);
    EXPECT_EQ(result->getChildren()[0]->getChildren()[0]->getType(), AstType::Register);
    EXPECT_EQ(result->getChildren()[0]->getChildren()[1]->getType(), AstType::Register);
    EXPECT_EQ(result->getChildren()[0]->getChildren()[2]->getType(), AstType::Value);
    EXPECT_EQ(result->getChildren()[0]->getChildren()[3]->getType(), AstType::Value);
}

TEST(MemoryParserTest, TestMemoryReferenceDirect)
{
    auto result = BasicParserTest::testRule(false, grammar::direct(), "(100)");
    EXPECT_EQ(result->getChildren()[0]->getType(), AstType::Memory);
    EXPECT_EQ(result->getChildren()[0]->getChildren().size(), 1);
    EXPECT_EQ(result->getChildren()[0]->getChildren()[0]->getType(), AstType::Value);
}
TEST(MemoryParserTest, TestMemoryReferenceIndexScale)
{
    auto result = BasicParserTest::testRule(false, grammar::indexScale(), "(, %rcx, 4)");
    EXPECT_EQ(result->getChildren()[0]->getType(), AstType::Memory);
    EXPECT_EQ(result->getChildren()[0]->getChildren().size(), 2);
    EXPECT_EQ(result->getChildren()[0]->getChildren()[0]->getType(), AstType::Register);
    EXPECT_EQ(result->getChildren()[0]->getChildren()[1]->getType(), AstType::Value);
}

TEST(MemoryParserTest, TestMemoryReferenceNoRightParen)
{
    auto result = BasicParserTest::testRule(true, grammar::memory(), "%rcx, %rbx, 1, 4)");
    EXPECT_EQ(result->getChildren().size(), 0);
}

TEST(MemoryParserTest, TestMemoryReferenceNoRegister)
{
    auto result = BasicParserTest::testRule(true, grammar::baseIndexScaleDisplacement(), "(%rcx, rbx, 1, 4)");
    EXPECT_EQ(result->getChildren().size(), 0);
}

TEST(MemoryParserTest, TestMemoryReferenceNoComma)
{
    auto result = BasicParserTest::testRule(true, grammar::baseIndexScaleDisplacement(), "(%rcx 1, 4)");
    EXPECT_EQ(result->getChildren().size(), 0);
}

TEST(MemoryParserTest, TestMemoryReferenceDoubleComma)
{
    auto result = BasicParserTest::testRule(true, grammar::baseIndexScaleDisplacement(), "(%rcx, , %rbx, 1, 4)");
    EXPECT_EQ(result->getChildren().size(), 0);
}

TEST(MemoryParserTest, TestMemoryReferenceNoLeftParen)
{
    auto result = BasicParserTest::testRule(true, grammar::baseIndexScaleDisplacement(), "(%rcx, %rbx, 1, 4");
    EXPECT_EQ(result->getChildren().size(), 0);
}

TEST(MemoryParserTest, TestMemoryReferenceEmpty)
{
    auto result = BasicParserTest::testRule(true, grammar::baseIndexScaleDisplacement(), "( )");
    EXPECT_EQ(result->getChildren().size(), 0);
}