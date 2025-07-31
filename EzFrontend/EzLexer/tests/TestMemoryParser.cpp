#include "BasicParserTest.h"

TEST(MemoryParserTest, TestMemoryReferenceBase)
{
    auto result = BasicParserTest::testRule(false, NodeParsers::BaseMemory(), "(%rcx)");
    auto memoryNode = result->getChild(0);

    BasicParserTest::expectChildCount(1, memoryNode);
    BasicParserTest::expectChildNodeType(AstNodes::VirtualVariable().getId(), 0, memoryNode);
}

TEST(MemoryParserTest, TestMemoryReferenceBaseDispl)
{
    auto result = BasicParserTest::testRule(false, NodeParsers::BaseDisplMemory(), "(%rcx, 4)");
    auto memoryNode = result->getChild(0);

    BasicParserTest::expectChildCount(2, memoryNode);
    BasicParserTest::expectChildNodeType(AstNodes::VirtualVariable().getId(), 0, memoryNode);
    BasicParserTest::expectChildNodeType(AstNodes::IntNumber().getId(), 1, memoryNode);
}

TEST(MemoryParserTest, TestMemoryReferenceBaseIndexScaleDispl)
{
    auto result = BasicParserTest::testRule(false, NodeParsers::BaseIndexScaleDisplMemory(), "(%rcx, %rbx, 1, 4)");
    auto memoryNode = result->getChild(0);

    BasicParserTest::expectChildCount(4, memoryNode);
    BasicParserTest::expectChildNodeType(AstNodes::VirtualVariable().getId(), 0, memoryNode);
    BasicParserTest::expectChildNodeType(AstNodes::VirtualVariable().getId(), 1, memoryNode);
    BasicParserTest::expectChildNodeType(AstNodes::IntNumber().getId(), 2, memoryNode);
    BasicParserTest::expectChildNodeType(AstNodes::IntNumber().getId(), 3, memoryNode);
}

TEST(MemoryParserTest, TestMemoryReferenceDirect)
{
    auto result = BasicParserTest::testRule(false, NodeParsers::DirectMemory(), "(100)");
    auto memoryNode = result->getChild(0);

    BasicParserTest::expectChildCount(1, memoryNode);
    BasicParserTest::expectChildNodeType(AstNodes::IntNumber().getId(), 0, memoryNode);
}
TEST(MemoryParserTest, TestMemoryReferenceIndexScale)
{
    auto result = BasicParserTest::testRule(false, NodeParsers::IndexScaleMemory(), "(, %rcx, 4)");
    auto memoryNode = result->getChild(0);

    BasicParserTest::expectChildCount(2, memoryNode);
    BasicParserTest::expectChildNodeType(AstNodes::VirtualVariable().getId(), 0, memoryNode);
    BasicParserTest::expectChildNodeType(AstNodes::IntNumber().getId(), 1, memoryNode);
}

TEST(MemoryParserTest, TestMemoryReferenceNoRightParen)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::MemoryReference(), "%rcx, %rbx, 1, 4)");
    BasicParserTest::expectChildCount(0, result);
}

TEST(MemoryParserTest, TestMemoryReferenceNoRegister)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::BaseIndexScaleDisplMemory(), "(%rcx, rbx, 1, 4)");
    BasicParserTest::expectChildCount(0, result);
}

TEST(MemoryParserTest, TestMemoryReferenceNoComma)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::BaseIndexScaleDisplMemory(), "(%rcx 1, 4)");
    BasicParserTest::expectChildCount(0, result);
}

TEST(MemoryParserTest, TestMemoryReferenceDoubleComma)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::BaseIndexScaleDisplMemory(), "(%rcx, , %rbx, 1, 4)");
    BasicParserTest::expectChildCount(0, result);
}

TEST(MemoryParserTest, TestMemoryReferenceNoLeftParen)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::BaseIndexScaleDisplMemory(), "(%rcx, %rbx, 1, 4");
    BasicParserTest::expectChildCount(0, result);
}

TEST(MemoryParserTest, TestMemoryReferenceEmpty)
{
    auto result = BasicParserTest::testRule(true, NodeParsers::BaseIndexScaleDisplMemory(), "( )");
    BasicParserTest::expectChildCount(0, result);
}