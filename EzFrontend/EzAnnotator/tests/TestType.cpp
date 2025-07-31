#include "BasicAnnotatorTest.h"

TEST_F(BasicAnnotatorTest, TestTypeDefined1)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::Type(), ".i8");
    std::shared_ptr<TypeAnnotator> ttAnnotator = std::make_shared<TypeAnnotator>(m_typeTable);
    EXPECT_NE(node, nullptr);

    std::shared_ptr<AstNode> typeNode = node->getChild(0);
    EXPECT_TRUE(ttAnnotator->annotate(typeNode, m_logger));
    EXPECT_TRUE(expectTypeId(m_i8TypeId, typeNode));
}

TEST_F(BasicAnnotatorTest, TestTypeDefined2)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::Type(), ".i16");
    std::shared_ptr<TypeAnnotator> ttAnnotator = std::make_shared<TypeAnnotator>(m_typeTable);
    EXPECT_NE(node, nullptr);

    std::shared_ptr<AstNode> typeNode = node->getChild(0);
    EXPECT_TRUE(ttAnnotator->annotate(typeNode, m_logger));
    EXPECT_TRUE(expectTypeId(m_i16TypeId, typeNode));
}

TEST_F(BasicAnnotatorTest, TestTypeDefined3)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::Type(), ".i32");
    std::shared_ptr<TypeAnnotator> ttAnnotator = std::make_shared<TypeAnnotator>(m_typeTable);
    EXPECT_NE(node, nullptr);

    std::shared_ptr<AstNode> typeNode = node->getChild(0);
    EXPECT_TRUE(ttAnnotator->annotate(typeNode, m_logger));
    EXPECT_TRUE(expectTypeId(m_i32TypeId, typeNode));
}

TEST_F(BasicAnnotatorTest, TestTypeDefined4)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::Type(), ".i64");
    std::shared_ptr<TypeAnnotator> ttAnnotator = std::make_shared<TypeAnnotator>(m_typeTable);
    EXPECT_NE(node, nullptr);

    std::shared_ptr<AstNode> typeNode = node->getChild(0);
    EXPECT_TRUE(ttAnnotator->annotate(typeNode, m_logger));
    EXPECT_TRUE(expectTypeId(m_i64TypeId, typeNode));
}

TEST_F(BasicAnnotatorTest, TestTypeUndefined)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::Type(), ".noReal");
    std::shared_ptr<TypeAnnotator> ttAnnotator = std::make_shared<TypeAnnotator>(m_typeTable);
    EXPECT_NE(node, nullptr);

    std::shared_ptr<AstNode> typeNode = node->getChild(0);
    EXPECT_FALSE(ttAnnotator->annotate(typeNode, m_logger));
}
