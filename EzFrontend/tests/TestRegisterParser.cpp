#include "BasicParserTest.h"

TEST(RegisterParserTest, ValidRegister)
{
    auto node = BasicParserTest::testRule(false, grammar::virtualVariable(), "%rcx");
    EXPECT_EQ(node->getChildren().size(), 1);
    EXPECT_EQ(node->getChildren()[0]->getType(), AstType::VirtualVariable);
}

TEST(RegisterParserTest, ValidRegister2)
{
    auto node = BasicParserTest::testRule(false, grammar::virtualVariable(), "%rcewrwerwex_312312312");
    EXPECT_EQ(node->getChildren().size(), 1);
    EXPECT_EQ(node->getChildren()[0]->getType(), AstType::VirtualVariable);
}

TEST(RegisterParserTest, ValidRegister3)
{
    auto node = BasicParserTest::testRule(false, grammar::virtualVariable(), "%a_21312313213");
    EXPECT_EQ(node->getChildren().size(), 1);
    EXPECT_EQ(node->getChildren()[0]->getType(), AstType::VirtualVariable);
}

TEST(RegisterParserTest, InvalidRegister)
{
    auto node = BasicParserTest::testRule(true, grammar::virtualVariable(), "%----"); // tokenize error
    EXPECT_EQ(node, nullptr);
}

TEST(RegisterParserTest, InvalidRegister2)
{
    auto node = BasicParserTest::testRule(true, grammar::virtualVariable(), "%");
    EXPECT_EQ(node->getChildren().size(), 0);
}

TEST(RegisterParserTest, InvalidRegister3)
{
    auto node = BasicParserTest::testRule(true, grammar::virtualVariable(), "%21221");
    EXPECT_EQ(node->getChildren().size(), 0);
}