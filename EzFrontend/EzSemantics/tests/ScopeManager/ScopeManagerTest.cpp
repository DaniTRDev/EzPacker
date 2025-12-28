#include "ScopeManagerTestFixture.h"

TEST_F(ScopeManagerTestFixture, SingleScope)
{
    m_scopeManager->beginScope();
    size_t id = m_scopeManager->getCurrentScopeId();
    m_scopeManager->endScope();

    TEST_SCOPE(id);
}

TEST_F(ScopeManagerTestFixture, SingleScopeNested)
{
    size_t root = 0, child = 0;
    m_scopeManager->beginScope();
    {
        root = m_scopeManager->getCurrentScopeId();
        m_scopeManager->beginScope();
        {
            child = m_scopeManager->getCurrentScopeId();
        }
        m_scopeManager->endScope();
    }
    m_scopeManager->endScope();

    TEST_CURRENT_SCOPE(root);
    EXPECT_NE(root, child);
}

TEST_F(ScopeManagerTestFixture, SingleScopeAllSymbols)
{
    m_scopeManager->beginScope();
    {
        EXPECT_TRUE(
                m_scopeManager->createSymbolAtCurrentScope(SymbolType::GlobalVariable, "mySymbol1", "i32", nullptr));
        EXPECT_TRUE(m_scopeManager->createSymbolAtCurrentScope(SymbolType::Label, "mySymbol2", "", nullptr));
        EXPECT_TRUE(m_scopeManager->createSymbolAtCurrentScope(SymbolType::LocalVariable, "mySymbol3", "i32", nullptr));
        EXPECT_TRUE(m_scopeManager->createSymbolAtCurrentScope(SymbolType::Module, "mySymbol4", "i32", nullptr));

        TEST_CURRENT_SCOPE_SYMBOL(m_scopeManager->getCurrentScopeId(), SymbolType::GlobalVariable, "mySymbol", "i32");
        TEST_CURRENT_SCOPE_SYMBOL(m_scopeManager->getCurrentScopeId(), SymbolType::Label, "mySymbol2", "");
        TEST_CURRENT_SCOPE_SYMBOL(m_scopeManager->getCurrentScopeId(), SymbolType::LocalVariable, "mySymbol3", "i32");
        TEST_CURRENT_SCOPE_SYMBOL(m_scopeManager->getCurrentScopeId(), SymbolType::Module, "mySymbol4", "i32");
    }
    m_scopeManager->endScope();
}

TEST_F(ScopeManagerTestFixture, SingleScopeRedefinition)
{
    m_scopeManager->beginScope();
    {
        // Same symbol type
        EXPECT_TRUE(
                m_scopeManager->createSymbolAtCurrentScope(SymbolType::GlobalVariable, "mySymbol1", "i32", nullptr));
        EXPECT_FALSE(
                m_scopeManager->createSymbolAtCurrentScope(SymbolType::GlobalVariable, "mySymbol1", "i64", nullptr));

        // Same symbol type and data type.
        EXPECT_TRUE(m_scopeManager->createSymbolAtCurrentScope(SymbolType::LocalVariable, "mySymbol3", "i32", nullptr));
        EXPECT_FALSE(
                m_scopeManager->createSymbolAtCurrentScope(SymbolType::LocalVariable, "mySymbol3", "i32", nullptr));
    }
    m_scopeManager->endScope();
}

TEST_F(ScopeManagerTestFixture, NestedScopeRedefinition)
{
    // Redefines a symbol from root scope inside a nested scope.
    std::shared_ptr<Symbol> globalVar, localVar;
    m_scopeManager->beginScope();
    {
        EXPECT_TRUE(
                m_scopeManager->createSymbolAtCurrentScope(SymbolType::GlobalVariable, "mySymbol1", "i32", &globalVar));

        m_scopeManager->beginScope();
        {
            EXPECT_TRUE(m_scopeManager->createSymbolAtCurrentScope(SymbolType::LocalVariable,
                                                                   "mySymbol1",
                                                                   "i32",
                                                                   &localVar));
        }
        m_scopeManager->endScope();
    }
    m_scopeManager->endScope();
    EXPECT_NE(globalVar->getId(), localVar->getId());
}