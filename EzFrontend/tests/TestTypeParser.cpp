#include "BasicParserTest.h"

TEST(TypeRuleTests, ValidType)
{
    auto nodes = BasicParserTest::testRule(false, grammar::type(), ".i64");
    EXPECT_EQ(nodes->getChildren().size(), 1);
    EXPECT_EQ(nodes->getChildren()[0]->getType(), AstType::Type);
}

TEST(TypeRuleTests, ValidType2)
{
    auto nodes = BasicParserTest::testRule(false, grammar::type(), ".i32");
    EXPECT_EQ(nodes->getChildren().size(), 1);
    EXPECT_EQ(nodes->getChildren()[0]->getType(), AstType::Type);
}

TEST(TypeRuleTests, InvalidType)
{
    auto nodes = BasicParserTest::testRule(true, grammar::type(), "i64");
    EXPECT_EQ(nodes->getChildren().size(), 0);
}

TEST(TypeRuleTests, InvalidType2)
{
    auto nodes = BasicParserTest::testRule(true, grammar::type(), "i64");
    EXPECT_EQ(nodes->getChildren().size(), 0);
}

TEST(TypeRuleTests, InvalidType3)
{
    auto nodes = BasicParserTest::testRule(true, grammar::type(), "%i64");
    EXPECT_EQ(nodes->getChildren().size(), 0);
}

TEST(TypeRuleTests, InvalidType4)
{
    auto nodes = BasicParserTest::testRule(true, grammar::type(), ".");
    EXPECT_EQ(nodes->getChildren().size(), 0);
}