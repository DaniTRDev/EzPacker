#include "BasicAnnotatorTest.h"

TEST_F(BasicAnnotatorTest, TestConstantFloat1)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::FloatNumber(), "0.25");
    std::shared_ptr<ConstantAnnotator> ccAnnotator = std::make_shared<ConstantAnnotator>();

    std::shared_ptr<AstNode> ccNode = node->getChild(0);

    EXPECT_TRUE(ccAnnotator->annotate(ccNode, m_logger));
    EXPECT_TRUE(expectConstantFloatValue(0.25f, ccNode));
}

TEST_F(BasicAnnotatorTest, TestConstantFloat2)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::FloatNumber(), "1.25");
    std::shared_ptr<ConstantAnnotator> ccAnnotator = std::make_shared<ConstantAnnotator>();

    std::shared_ptr<AstNode> ccNode = node->getChild(0);

    EXPECT_TRUE(ccAnnotator->annotate(ccNode, m_logger));
    EXPECT_TRUE(expectConstantFloatValue(1.25f, ccNode));
}

TEST_F(BasicAnnotatorTest, TestConstantFloat3)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::FloatNumber(), "3.75");
    std::shared_ptr<ConstantAnnotator> ccAnnotator = std::make_shared<ConstantAnnotator>();

    std::shared_ptr<AstNode> ccNode = node->getChild(0);

    EXPECT_TRUE(ccAnnotator->annotate(ccNode, m_logger));
    EXPECT_TRUE(expectConstantFloatValue(3.75f, ccNode));
}

TEST_F(BasicAnnotatorTest, TestConstantInteger1)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::IntNumber(), "9191001");
    std::shared_ptr<ConstantAnnotator> ccAnnotator = std::make_shared<ConstantAnnotator>();

    std::shared_ptr<AstNode> ccNode = node->getChild(0);

    EXPECT_TRUE(ccAnnotator->annotate(ccNode, m_logger));
    EXPECT_TRUE(expectConstantIntegerValue(9191001, ccNode));
}

TEST_F(BasicAnnotatorTest, TestConstantInteger2)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::IntNumber(), "1337");
    std::shared_ptr<ConstantAnnotator> ccAnnotator = std::make_shared<ConstantAnnotator>();

    std::shared_ptr<AstNode> ccNode = node->getChild(0);

    EXPECT_TRUE(ccAnnotator->annotate(ccNode, m_logger));
    EXPECT_TRUE(expectConstantIntegerValue(1337, ccNode));
}

TEST_F(BasicAnnotatorTest, TestConstantInteger3)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::IntNumber(), "00012321");
    std::shared_ptr<ConstantAnnotator> ccAnnotator = std::make_shared<ConstantAnnotator>();

    std::shared_ptr<AstNode> ccNode = node->getChild(0);

    EXPECT_TRUE(ccAnnotator->annotate(ccNode, m_logger));
    EXPECT_TRUE(expectConstantIntegerValue(12321, ccNode));
}

TEST_F(BasicAnnotatorTest, TestConstantString1)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::String(), "\"HELLO\"");
    std::shared_ptr<ConstantAnnotator> ccAnnotator = std::make_shared<ConstantAnnotator>();

    std::shared_ptr<AstNode> ccNode = node->getChild(0);

    EXPECT_TRUE(ccAnnotator->annotate(ccNode, m_logger));
    EXPECT_TRUE(expectConstantStringValue("HELLO", ccNode));
}

TEST_F(BasicAnnotatorTest, TestConstantString2)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::String(), "\"BYE BYE\"");
    std::shared_ptr<ConstantAnnotator> ccAnnotator = std::make_shared<ConstantAnnotator>();

    std::shared_ptr<AstNode> ccNode = node->getChild(0);

    EXPECT_TRUE(ccAnnotator->annotate(ccNode, m_logger));
    EXPECT_TRUE(expectConstantStringValue("BYE BYE", ccNode));
}

TEST_F(BasicAnnotatorTest, TestConstantString3)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::String(), "\"TEST STRING\"");
    std::shared_ptr<ConstantAnnotator> ccAnnotator = std::make_shared<ConstantAnnotator>();

    std::shared_ptr<AstNode> ccNode = node->getChild(0);

    EXPECT_TRUE(ccAnnotator->annotate(ccNode, m_logger));
    EXPECT_TRUE(expectConstantStringValue("TEST STRING", ccNode));
}