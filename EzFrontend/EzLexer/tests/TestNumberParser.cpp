#include "BasicParserTest.h"

TEST(NumberParserTest, ValidInteger)
{
    auto result = BasicParserTest::testRule(false, NodeParsers::IntNumber(), "123");

    BasicParserTest::expectChildCount(1, result);
    BasicParserTest::expectChildNodeType(AstNodes::IntNumber().getId(), 0, result);
}

TEST(NumberParserTest, ValidFloat)
{
    auto result = BasicParserTest::testRule(false, NodeParsers::FloatNumber(), "123.2323");

    BasicParserTest::expectChildCount(1, result);
    BasicParserTest::expectChildNodeType(AstNodes::FloatNumber().getId(), 0, result);
}

TEST(NumberParserTest, InvalidFloatDoubleDot)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::FloatNumber(), "3.13.23");
    EXPECT_EQ(result, nullptr); // Tokenize error.
}

TEST(NumberParserTest, InvalidEmpty)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::FloatNumber(), "");
    EXPECT_EQ(result, nullptr); // Tokenize error.
}
