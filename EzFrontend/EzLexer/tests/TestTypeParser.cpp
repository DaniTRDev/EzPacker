#include "BasicParserTest.h"

TEST(TypeRuleTests, ValidType)
{
    auto result = BasicParserTest::testRule(false, NodeParsers::Type(), ".i64");
    BasicParserTest::expectChildCount(1, result);
    BasicParserTest::expectChildNodeType(AstNodes::Type().getId(), 0, result);
}

TEST(TypeRuleTests, ValidType2)
{
    auto result = BasicParserTest::testRule(false, NodeParsers::Type(), ".i32");
    BasicParserTest::expectChildCount(1, result);
    BasicParserTest::expectChildNodeType(AstNodes::Type().getId(), 0, result);
}

TEST(TypeRuleTests, InvalidType)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::Type(), "i64");
    BasicParserTest::expectChildCount(0, result);
}

TEST(TypeRuleTests, InvalidType2)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::Type(), "i64");
    BasicParserTest::expectChildCount(0, result);
}

TEST(TypeRuleTests, InvalidType3)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::Type(), "%i64");
    BasicParserTest::expectChildCount(0, result);
}

TEST(TypeRuleTests, InvalidType4)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::Type(), ".");
    BasicParserTest::expectChildCount(0, result);
}