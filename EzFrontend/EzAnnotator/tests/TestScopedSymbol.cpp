#include "BasicAnnotatorTest.h"

TEST_F(BasicAnnotatorTest, TestDefinedSymbol)
{
    std::shared_ptr<ScopedSymbol> symbol = std::make_shared<ScopedSymbol>(m_i8TypeId, "TestSymbol");
    size_t symbolId = m_symbolTable->addItem(symbol);

    EXPECT_TRUE(m_symbolTable->doesElemExist(symbolId));
    EXPECT_TRUE(m_symbolTable->doesElemExistAtAnyUpperScope(symbolId));
}

TEST_F(BasicAnnotatorTest, TestDefinedSymbolInOtherScope)
{
    std::shared_ptr<ScopedSymbol> symbol = std::make_shared<ScopedSymbol>(m_i8TypeId, "TestSymbol");
    size_t symbolId = m_symbolTable->addItem(symbol);

    EXPECT_TRUE(m_symbolTable->doesElemExist(symbolId));
    EXPECT_TRUE(m_symbolTable->doesElemExistAtAnyUpperScope(symbolId));

    m_symbolTable->beginScope();

    EXPECT_FALSE(m_symbolTable->doesElemExist(symbolId));
    EXPECT_TRUE(m_symbolTable->doesElemExistAtAnyUpperScope(symbolId));

    m_symbolTable->endScope();
}

TEST_F(BasicAnnotatorTest, TestUnDefinedSymbol)
{
    std::shared_ptr<ScopedSymbol> symbol = std::make_shared<ScopedSymbol>(m_i8TypeId, "TestSymbol");

    m_symbolTable->beginScope();
    size_t symbolId = m_symbolTable->addItem(symbol);
    EXPECT_TRUE(m_symbolTable->doesElemExist(symbolId));
    EXPECT_TRUE(m_symbolTable->doesElemExistAtAnyUpperScope(symbolId));
    m_symbolTable->endScope(); // Symbol should not be present any more after this line.

    EXPECT_FALSE(m_symbolTable->doesElemExist(symbolId));
    EXPECT_FALSE(m_symbolTable->doesElemExistAtAnyUpperScope(symbolId));
}