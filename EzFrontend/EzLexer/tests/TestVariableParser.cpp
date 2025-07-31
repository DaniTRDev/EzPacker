#include "BasicParserTest.h"

TEST(VariableParseTest, TestValidVariable)
{
    auto result = BasicParserTest::testRule(false, NodeParsers::Variable(), ".variable myVar: .float 3.12;");
    BasicParserTest::expectChildCount(1, result);
    BasicParserTest::expectChildNodeType(AstNodes::Variable().getId(), 0, result);

    auto variable = result->getChild(0);
    BasicParserTest::expectChildCount(3, variable);
    BasicParserTest::expectChildNodeType(AstNodes::Identifier().getId(), 0, variable);
    BasicParserTest::expectChildNodeType(AstNodes::Type().getId(), 1, variable);
    BasicParserTest::expectChildNodeType(AstNodes::FloatNumber().getId(), 2, variable);
}

TEST(VariableParseTest, TestValidVariable2)
{
    auto result = BasicParserTest::testRule(false,
                                            NodeParsers::Variable(),
                                            ".variable asdasda_asda: .float 3.12, 23.0, 321.0;");
    BasicParserTest::expectChildCount(1, result);
    BasicParserTest::expectChildNodeType(AstNodes::Variable().getId(), 0, result);

    auto variable = result->getChild(0);
    BasicParserTest::expectChildCount(5, variable);
    BasicParserTest::expectChildNodeType(AstNodes::Identifier().getId(), 0, variable);
    BasicParserTest::expectChildNodeType(AstNodes::Type().getId(), 1, variable);
    BasicParserTest::expectChildNodeType(AstNodes::FloatNumber().getId(), 2, variable);
    BasicParserTest::expectChildNodeType(AstNodes::FloatNumber().getId(), 3, variable);
    BasicParserTest::expectChildNodeType(AstNodes::FloatNumber().getId(), 4, variable);
}

TEST(VariableParseTest, TestValidVariable3)
{
    auto result = BasicParserTest::testRule(false, NodeParsers::Variable(), ".variable myVar: .i64 0, 0, 0, 1;");
    BasicParserTest::expectChildCount(1, result);
    BasicParserTest::expectChildNodeType(AstNodes::Variable().getId(), 0, result);

    auto variable = result->getChild(0);
    BasicParserTest::expectChildCount(6, variable);
    BasicParserTest::expectChildNodeType(AstNodes::Identifier().getId(), 0, variable);
    BasicParserTest::expectChildNodeType(AstNodes::Type().getId(), 1, variable);
    BasicParserTest::expectChildNodeType(AstNodes::IntNumber().getId(), 2, variable);
    BasicParserTest::expectChildNodeType(AstNodes::IntNumber().getId(), 3, variable);
    BasicParserTest::expectChildNodeType(AstNodes::IntNumber().getId(), 4, variable);
    BasicParserTest::expectChildNodeType(AstNodes::IntNumber().getId(), 5, variable);
}

TEST(VariableParseTest, TestValidVariable4)
{
    auto result = BasicParserTest::testRule(false, NodeParsers::Variable(), ".variable myVar: .i8 \"sada\", \"asd\";");
    BasicParserTest::expectChildCount(1, result);
    BasicParserTest::expectChildNodeType(AstNodes::Variable().getId(), 0, result);

    auto variable = result->getChild(0);
    BasicParserTest::expectChildCount(4, variable);
    BasicParserTest::expectChildNodeType(AstNodes::Identifier().getId(), 0, variable);
    BasicParserTest::expectChildNodeType(AstNodes::Type().getId(), 1, variable);
    BasicParserTest::expectChildNodeType(AstNodes::String().getId(), 2, variable);
    BasicParserTest::expectChildNodeType(AstNodes::String().getId(), 3, variable);
}

TEST(VariableParseTest, TestInvalidKeyword)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::Variable(), "asd myVar: .i64 0, 0, 0, 1;");
    BasicParserTest::expectChildCount(0, result);
}

TEST(VariableParseTest, TestInvalidName)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::Variable(), ".variable .myVar: .i64 0, 0, 0, 1;");
    BasicParserTest::expectChildCount(0, result);
}

TEST(VariableParseTest, TestInvalidName2)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::Variable(), ".variable myVar-asd: .i64 0, 0, 0, 1;");
    EXPECT_EQ(result, nullptr); // Invalid token output.
}

TEST(VariableParseTest, TestInvalidName3)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::Variable(), ".variable %myVar: .i64 0, 0, 0, 1;");
    BasicParserTest::expectChildCount(0, result);
}

TEST(VariableParseTest, TestInvalidType)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::Variable(), ".variable myVar: %i64 0, 0, 0, 1;");
    BasicParserTest::expectChildCount(0, result);
}

TEST(VariableParseTest, TestInvalidEmptyInitializer)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::Variable(), ".variable myVar: .i64 ;");
    BasicParserTest::expectChildCount(0, result);
}

TEST(VariableParseTest, TestInvalidInitializer)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::Variable(), ".variable myVar: .i64 .keyword ;");
    BasicParserTest::expectChildCount(0, result);
}

TEST(VariableParseTest, TestInvalidInitializer2)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::Variable(), ".variable myVar: .i64 %rax ;");
    BasicParserTest::expectChildCount(0, result);
}

TEST(VariableParseTest, TestInvalidInitializer3)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::Variable(), ".variable myVar: .i64 @ ;");
    EXPECT_EQ(result, nullptr); // Tokenize error.
}