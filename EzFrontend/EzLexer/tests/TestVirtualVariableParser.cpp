#include "BasicParserTest.h"

TEST(RegisterParserTest, ValidRegister)
{
    auto result = BasicParserTest::testRule(false, NodeParsers::VirtualVariable(), "%rcx");
    BasicParserTest::expectChildCount(1, result);
    BasicParserTest::expectChildNodeType(AstNodes::VirtualVariable().getId(), 0, result);
}

TEST(RegisterParserTest, ValidRegister2)
{
    auto result = BasicParserTest::testRule(false, NodeParsers::VirtualVariable(), "%rcewrwerwex_312312312");
    BasicParserTest::expectChildCount(1, result);
    BasicParserTest::expectChildNodeType(AstNodes::VirtualVariable().getId(), 0, result);
}

TEST(RegisterParserTest, ValidRegister3)
{
    auto result = BasicParserTest::testRule(false, NodeParsers::VirtualVariable(), "%a_21312313213");
    BasicParserTest::expectChildCount(1, result);
    BasicParserTest::expectChildNodeType(AstNodes::VirtualVariable().getId(), 0, result);
}

TEST(RegisterParserTest, InvalidRegister)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::VirtualVariable(), "%----"); // tokenizeBuffer error
    EXPECT_EQ(result, nullptr);
}

TEST(RegisterParserTest, InvalidRegister2)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::VirtualVariable(), "%");
    BasicParserTest::expectChildCount(0, result);
}

TEST(RegisterParserTest, InvalidRegister3)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::VirtualVariable(), "%21221");
    BasicParserTest::expectChildCount(0, result);
}