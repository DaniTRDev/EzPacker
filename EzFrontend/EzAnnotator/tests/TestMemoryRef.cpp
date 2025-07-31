#include "BasicAnnotatorTest.h"

TEST_F(BasicAnnotatorTest, TestBaseMemoryRef)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::BaseMemory(), "(%rcx)");
    std::shared_ptr<ScopedSymbol> rcx = std::make_shared<ScopedSymbol>(m_i64TypeId, "rcx");
    std::shared_ptr<MemoryRefAnnotator> mmAnotator = std::make_shared<MemoryRefAnnotator>(m_symbolTable);
    EXPECT_NE(node, nullptr);

    size_t rcxId = m_symbolTable->addItem(rcx); // Add the symbol.
    std::shared_ptr<AstNode> memoryRefNode = node->getChild(0);

    mmAnotator->setMemoryType(m_i64TypeId);
    EXPECT_TRUE(mmAnotator->annotate(node->getChild(0), m_logger));
    EXPECT_TRUE(expectMemoryBase(rcxId, m_i64TypeId, memoryRefNode));
}

TEST_F(BasicAnnotatorTest, TestBaseDisplMemoryRef)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::BaseDisplMemory(), "(%rcx, 1000)");
    std::shared_ptr<ScopedSymbol> rcx = std::make_shared<ScopedSymbol>(m_i64TypeId, "rcx");
    std::shared_ptr<MemoryRefAnnotator> mmAnotator = std::make_shared<MemoryRefAnnotator>(m_symbolTable);
    EXPECT_NE(node, nullptr);

    size_t rcxId = m_symbolTable->addItem(rcx); // Add the symbol.
    std::shared_ptr<AstNode> memoryRefNode = node->getChild(0);

    mmAnotator->setMemoryType(m_i64TypeId);
    EXPECT_TRUE(mmAnotator->annotate(node->getChild(0), m_logger));
    EXPECT_TRUE(expectMemoryBaseDispl(rcxId, 1000, m_i64TypeId, memoryRefNode));
}

TEST_F(BasicAnnotatorTest, TestBaseIndexScaleDisplMemoryRef)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::BaseIndexScaleDisplMemory(), "(%rcx, %rdx, 8, 1000)");
    std::shared_ptr<ScopedSymbol> rcx = std::make_shared<ScopedSymbol>(m_i64TypeId, "rcx");
    std::shared_ptr<ScopedSymbol> rdx = std::make_shared<ScopedSymbol>(m_i64TypeId, "rdx");
    std::shared_ptr<MemoryRefAnnotator> mmAnotator = std::make_shared<MemoryRefAnnotator>(m_symbolTable);
    EXPECT_NE(node, nullptr);

    size_t rcxId = m_symbolTable->addItem(rcx); // Add the symbol.
    size_t rdxId = m_symbolTable->addItem(rdx); // Add the symbol.
    std::shared_ptr<AstNode> memoryRefNode = node->getChild(0);

    mmAnotator->setMemoryType(m_i64TypeId);
    EXPECT_TRUE(mmAnotator->annotate(node->getChild(0), m_logger));
    EXPECT_TRUE(expectMemoryBaseIndexScaleDispl(rcxId, rdxId, 8, 1000, m_i64TypeId, memoryRefNode));
}

TEST_F(BasicAnnotatorTest, TestDirectMemoryRef)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::DirectMemory(), "(99991201)");
    std::shared_ptr<MemoryRefAnnotator> mmAnotator = std::make_shared<MemoryRefAnnotator>(m_symbolTable);
    EXPECT_NE(node, nullptr);

    std::shared_ptr<AstNode> memoryRefNode = node->getChild(0);

    mmAnotator->setMemoryType(m_i64TypeId);
    EXPECT_TRUE(mmAnotator->annotate(node->getChild(0), m_logger));
    EXPECT_TRUE(expectMemoryDirect(99991201, m_i64TypeId, memoryRefNode));
}

TEST_F(BasicAnnotatorTest, TestScaleIndexMemoryRef)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::IndexScaleMemory(), "(, %rdx, 8)");
    std::shared_ptr<ScopedSymbol> rdx = std::make_shared<ScopedSymbol>(m_i64TypeId, "rdx");
    std::shared_ptr<MemoryRefAnnotator> mmAnotator = std::make_shared<MemoryRefAnnotator>(m_symbolTable);
    EXPECT_NE(node, nullptr);

    size_t rdxId = m_symbolTable->addItem(rdx); // Add the symbol.
    std::shared_ptr<AstNode> memoryRefNode = node->getChild(0);

    mmAnotator->setMemoryType(m_i64TypeId);
    EXPECT_TRUE(mmAnotator->annotate(node->getChild(0), m_logger));
    EXPECT_TRUE(expectMemoryIndexScale(rdxId, 8, m_i64TypeId, memoryRefNode));
}