#include "BasicParserTest.h"

TEST(NumberParserTest, ValidInteger)
{
    auto nodes = BasicParserTest::testRule(false, grammar::number(), "123");

    EXPECT_EQ(nodes->getChildren().size(), 1);
    EXPECT_EQ(nodes->getChildren()[0]->getChildren().size(), 1);
    EXPECT_EQ(nodes->getChildren()[0]->getType(), AstType::Value);
}

TEST(NumberParserTest, ValidFloat)
{
    auto nodes = BasicParserTest::testRule(false, grammar::number(), "123.2323");
    
    EXPECT_EQ(nodes->getChildren().size(), 1);
    EXPECT_EQ(nodes->getChildren()[0]->getChildren().size(), 1);
    EXPECT_EQ(nodes->getChildren()[0]->getType(), AstType::Value);
}

TEST(NumberParserTest, InvalidFloatDoubleDot)
{
    auto nodes = BasicParserTest::testRule(true, grammar::number(), "3.13.23");
    EXPECT_EQ(nodes, nullptr);
}

TEST(NumberParserTest, InvalidEmpty)
{
    auto nodes = BasicParserTest::testRule(true, grammar::number(), "");
    EXPECT_EQ(nodes, nullptr);
}
