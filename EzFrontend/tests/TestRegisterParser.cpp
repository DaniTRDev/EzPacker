#include "BasicParserTest.h"

TEST(RegisterParserTest, ValidRegister)
{
    auto node = BasicParserTest::testRule(false, grammar::_register(), "%rcx");
    EXPECT_EQ(node->getChildren().size(), 1);
    EXPECT_EQ(node->getChildren()[0]->getType(), AstType::Register);
}

TEST(RegisterParserTest, ValidRegister2)
{
    auto node = BasicParserTest::testRule(false, grammar::_register(), "%rcewrwerwex_312312312");
    EXPECT_EQ(node->getChildren().size(), 1);
    EXPECT_EQ(node->getChildren()[0]->getType(), AstType::Register);
}

TEST(RegisterParserTest, ValidRegister3)
{
    auto node = BasicParserTest::testRule(false, grammar::_register(), "%a_21312313213");
    EXPECT_EQ(node->getChildren().size(), 1);
    EXPECT_EQ(node->getChildren()[0]->getType(), AstType::Register);
}

TEST(RegisterParserTest, InvalidRegister)
{
    auto node = BasicParserTest::testRule(true, grammar::_register(), "%----"); // tokenize error
    EXPECT_EQ(node, nullptr);
}

TEST(RegisterParserTest, InvalidRegister2)
{
    auto node = BasicParserTest::testRule(true, grammar::_register(), "%");
    EXPECT_EQ(node->getChildren().size(), 0);
}

TEST(RegisterParserTest, InvalidRegister3)
{
    auto node = BasicParserTest::testRule(true, grammar::_register(), "%21221");
    EXPECT_EQ(node->getChildren().size(), 0);
}