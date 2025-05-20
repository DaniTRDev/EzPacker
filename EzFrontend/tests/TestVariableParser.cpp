#include "BasicParserTest.h"

TEST(VariableParseTest, TestValidVariable)
{
    auto nodes = BasicParserTest::testRule(false, grammar::variable(), ".variable myVar: .float 3.12");
    EXPECT_EQ(nodes->getChildren().size(), 1);

    auto variable = std::dynamic_pointer_cast<VariableNode>(nodes->getChildren()[0]);
    EXPECT_EQ(variable->getChildren().size(), 4);
    EXPECT_EQ(variable->getChildren()[0]->getType(), AstType::Identifier); // keyword also counts.
    EXPECT_EQ(variable->getChildren()[1]->getType(), AstType::Identifier);
    EXPECT_EQ(variable->getChildren()[2]->getType(), AstType::Type);
    EXPECT_EQ(variable->getChildren()[3]->getType(), AstType::Value);
}

TEST(VariableParseTest, TestValidVariable2)
{
    auto nodes =
        BasicParserTest::testRule(false, grammar::variable(), ".variable asdasda_asda: .float 3.12, 23.0, 321.0");
    EXPECT_EQ(nodes->getChildren().size(), 1);

    auto variable = std::dynamic_pointer_cast<VariableNode>(nodes->getChildren()[0]);
    EXPECT_EQ(variable->getChildren().size(), 6);
    EXPECT_EQ(variable->getChildren()[0]->getType(), AstType::Identifier); // Keyword also counts
    EXPECT_EQ(variable->getChildren()[1]->getType(), AstType::Identifier);
    EXPECT_EQ(variable->getChildren()[2]->getType(), AstType::Type);
    EXPECT_EQ(variable->getChildren()[3]->getType(), AstType::Value);
    EXPECT_EQ(variable->getChildren()[4]->getType(), AstType::Value);
    EXPECT_EQ(variable->getChildren()[5]->getType(), AstType::Value);
}

TEST(VariableParseTest, TestValidVariable3)
{
    auto nodes = BasicParserTest::testRule(false, grammar::variable(), ".variable myVar: .i64 0, 0, 0, 1");
    EXPECT_EQ(nodes->getChildren().size(), 1);

    auto variable = std::dynamic_pointer_cast<VariableNode>(nodes->getChildren()[0]);
    EXPECT_EQ(variable->getChildren().size(), 7);
    EXPECT_EQ(variable->getChildren()[0]->getType(), AstType::Identifier); // Keyword also counts.
    EXPECT_EQ(variable->getChildren()[1]->getType(), AstType::Identifier);
    EXPECT_EQ(variable->getChildren()[2]->getType(), AstType::Type);
    EXPECT_EQ(variable->getChildren()[3]->getType(), AstType::Value);
    EXPECT_EQ(variable->getChildren()[4]->getType(), AstType::Value);
    EXPECT_EQ(variable->getChildren()[5]->getType(), AstType::Value);
    EXPECT_EQ(variable->getChildren()[6]->getType(), AstType::Value);
}

TEST(VariableParseTest, TestValidVariable4)
{
    auto nodes = BasicParserTest::testRule(false, grammar::variable(), ".variable myVar: .string \"sada\", \"asd\" ");
    EXPECT_EQ(nodes->getChildren().size(), 1);

    auto variable = std::dynamic_pointer_cast<VariableNode>(nodes->getChildren()[0]);
    EXPECT_EQ(variable->getChildren().size(), 5);
    EXPECT_EQ(variable->getChildren()[0]->getType(), AstType::Identifier); // Keyword also counts.
    EXPECT_EQ(variable->getChildren()[1]->getType(), AstType::Identifier);
    EXPECT_EQ(variable->getChildren()[2]->getType(), AstType::Type);
    EXPECT_EQ(variable->getChildren()[3]->getType(), AstType::Value);
    EXPECT_EQ(variable->getChildren()[4]->getType(), AstType::Value);
}

TEST(VariableParseTest, TestInvalidKeyword)
{
    auto nodes = BasicParserTest::testRule(true, grammar::variable(), "asd myVar: .i64 0, 0, 0, 1");
    EXPECT_EQ(nodes->getChildren().size(), 0);
}

TEST(VariableParseTest, TestInvalidName)
{
    auto nodes = BasicParserTest::testRule(true, grammar::variable(), ".variable .myVar: .i64 0, 0, 0, 1");
    EXPECT_EQ(nodes->getChildren().size(), 0);
}

TEST(VariableParseTest, TestInvalidName2)
{
    auto nodes = BasicParserTest::testRule(true, grammar::variable(), ".variable myVar-asd: .i64 0, 0, 0, 1");
    EXPECT_EQ(nodes, nullptr); // Invalid token output.
}

TEST(VariableParseTest, TestInvalidName3)
{
    auto nodes = BasicParserTest::testRule(true, grammar::variable(), ".variable %myVar: .i64 0, 0, 0, 1");
    EXPECT_EQ(nodes->getChildren().size(), 0);
}

TEST(VariableParseTest, TestInvalidType)
{
    auto nodes = BasicParserTest::testRule(true, grammar::variable(), ".variable myVar: %i64 0, 0, 0, 1");
    EXPECT_EQ(nodes->getChildren().size(), 0);
}

TEST(VariableParseTest, TestInvalidEmptyInitializer)
{
    auto nodes = BasicParserTest::testRule(true, grammar::variable(), ".variable myVar: .i64");
    EXPECT_EQ(nodes->getChildren().size(), 0);
}

TEST(VariableParseTest, TestInvalidInitializer)
{
    auto nodes = BasicParserTest::testRule(true, grammar::variable(), ".variable myVar: .i64 .keyword");
    EXPECT_EQ(nodes->getChildren().size(), 0);
}

TEST(VariableParseTest, TestInvalidInitializer2)
{
    auto nodes = BasicParserTest::testRule(true, grammar::variable(), ".variable myVar: .i64 %rax");
    EXPECT_EQ(nodes->getChildren().size(), 0);
}

TEST(VariableParseTest, TestInvalidInitializer3)
{
    auto nodes = BasicParserTest::testRule(true, grammar::variable(), ".variable myVar: .i64 @");
    EXPECT_EQ(nodes, nullptr); // Tokenize error.
}