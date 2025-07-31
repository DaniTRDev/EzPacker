#include "BasicAnnotatorTest.h"

TEST_F(BasicAnnotatorTest, TestVirtualVariableCheckExistingValid)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::VirtualVariable(), "%rcx");
    std::shared_ptr<ScopedSymbol> symbol = std::make_shared<ScopedSymbol>(m_i8TypeId, "rcx");
    std::shared_ptr<VirtualVariableAnnotator> vvAnotator = std::make_shared<VirtualVariableAnnotator>(m_symbolTable);
    EXPECT_NE(node, nullptr);

    size_t symbolId = m_symbolTable->addItem(symbol); // Add the symbol.
    std::shared_ptr<AstNode> vvNode = node->getChild(0);

    vvAnotator->setTypeId(4);
    vvAnotator->setWorkingMode(VVAnnotatorWorkingMode::ExpectsExistingSymbol);
    EXPECT_TRUE(vvAnotator->annotate(vvNode, m_logger));
    EXPECT_TRUE(expectSymbolId(symbolId, vvNode));
    EXPECT_TRUE(expectSymbolTypeId(4, vvNode));
}

TEST_F(BasicAnnotatorTest, TestVirtualVariableCheckExistingInvalid)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::VirtualVariable(), "%rcx");
    std::shared_ptr<VirtualVariableAnnotator> vvAnotator = std::make_shared<VirtualVariableAnnotator>(m_symbolTable);
    EXPECT_NE(node, nullptr);

    std::shared_ptr<AstNode> vvNode = node->getChild(0);

    vvAnotator->setTypeId(4);
    vvAnotator->setWorkingMode(VVAnnotatorWorkingMode::ExpectsExistingSymbol);
    EXPECT_FALSE(vvAnotator->annotate(vvNode, m_logger)); // Undefined symbol.
}

TEST_F(BasicAnnotatorTest, TestVirtualVariableCreateNewValid)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::VirtualVariable(), "%rcx");
    std::shared_ptr<VirtualVariableAnnotator> vvAnotator = std::make_shared<VirtualVariableAnnotator>(m_symbolTable);
    EXPECT_NE(node, nullptr);

    size_t symbolId = 0;
    std::shared_ptr<AstNode> vvNode = node->getChild(0);

    vvAnotator->setTypeId(4);
    vvAnotator->setWorkingMode(VVAnnotatorWorkingMode::CreateNewSymbol);
    EXPECT_TRUE(vvAnotator->annotate(node->getChild(0), m_logger));
    EXPECT_TRUE(expectSymbolTypeId(4, vvNode));

    EXPECT_TRUE(m_symbolTable->doesElemExistAtAnyUpperScopeByString(ScopedSymbol::getStringFromName("rcx"), symbolId));
    EXPECT_TRUE(expectSymbolId(symbolId, vvNode));
}

TEST_F(BasicAnnotatorTest, TestVirtualVariableCreateNewInvalid)
{
    std::shared_ptr<AstNode> node = parse(NodeParsers::VirtualVariable(), "%rcx");
    std::shared_ptr<ScopedSymbol> symbol = std::make_shared<ScopedSymbol>(m_i8TypeId, "rcx");
    std::shared_ptr<VirtualVariableAnnotator> vvAnotator = std::make_shared<VirtualVariableAnnotator>(m_symbolTable);
    EXPECT_NE(node, nullptr);

    size_t symbolId = m_symbolTable->addItem(symbol);
    std::shared_ptr<AstNode> vvNode = node->getChild(0);

    vvAnotator->setTypeId(4);
    vvAnotator->setWorkingMode(VVAnnotatorWorkingMode::CreateNewSymbol);
    EXPECT_FALSE(vvAnotator->annotate(node->getChild(0), m_logger)); // Symbol redefinition

    // Check that symbol type wasn't changed.
    EXPECT_EQ(m_symbolTable->getItem<ScopedSymbol>(symbolId)->getTypeId(), m_i8TypeId);
}
