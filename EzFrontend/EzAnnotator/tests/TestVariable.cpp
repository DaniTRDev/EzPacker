#include "BasicAnnotatorTest.h"

TEST_F(BasicAnnotatorTest, TestVariableSymbolAndType)
{
    std::string input = R"(.variable myVar: .i64 8;)";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Variable(), input);
    std::shared_ptr<VariableAnnotator> vvAnnotator = std::make_shared<VariableAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    size_t variableSymbolId = 0;
    std::shared_ptr<AstNode> variableNode = node->getChild(0);

    EXPECT_TRUE(vvAnnotator->annotate(variableNode, m_logger));
    EXPECT_TRUE(
            m_symbolTable->doesElemExistAtAnyScopeByString(ScopedSymbol::getStringFromName("myVar"), variableSymbolId));
}

TEST_F(BasicAnnotatorTest, TestVariable1FloatArg)
{
    std::string input = R"(.variable myVar: .float 3.5;)";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Variable(), input);
    std::shared_ptr<VariableAnnotator> vvAnnotator = std::make_shared<VariableAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    size_t variableSymbolId = 0;
    std::shared_ptr<AstNode> variableNode = node->getChild(0);

    EXPECT_TRUE(vvAnnotator->annotate(variableNode, m_logger));
    EXPECT_TRUE(expectVariableInitializerCount(1, variableNode));
    EXPECT_TRUE(expectVariableFloatValue(0, 3.5, variableNode));
}

TEST_F(BasicAnnotatorTest, TestVariable1IntArg)
{
    std::string input = R"(.variable myVar: .i64 1337;)";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Variable(), input);
    std::shared_ptr<VariableAnnotator> vvAnnotator = std::make_shared<VariableAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    size_t variableSymbolId = 0;
    std::shared_ptr<AstNode> variableNode = node->getChild(0);

    EXPECT_TRUE(vvAnnotator->annotate(variableNode, m_logger));
    EXPECT_TRUE(expectVariableInitializerCount(1, variableNode));
    EXPECT_TRUE(expectVariableIntValue(0, 1337, variableNode));
}

TEST_F(BasicAnnotatorTest, TestVariable1StringArg)
{
    std::string input = R"(.variable myVar: .string "test";)";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Variable(), input);
    std::shared_ptr<VariableAnnotator> vvAnnotator = std::make_shared<VariableAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    size_t variableSymbolId = 0;
    std::shared_ptr<AstNode> variableNode = node->getChild(0);

    EXPECT_TRUE(vvAnnotator->annotate(variableNode, m_logger));
    EXPECT_TRUE(expectVariableInitializerCount(1, variableNode));
    EXPECT_TRUE(expectVariableStringValue(0, "test", variableNode));
}