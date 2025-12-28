#include "ScopeManagerTestFixture.h"

void ScopeManagerTestFixture::SetUp()
{
    Test::SetUp();
    m_scopeManager = std::make_shared<ScopeManager>(std::map<size_t, std::shared_ptr<Scope>>{});
}

void ScopeManagerTestFixture::TearDown()
{
    Test::TearDown();
    m_scopeManager.reset();
}

bool ScopeManagerTestFixture::expectCurrentScopeId(size_t id) { return m_scopeManager->getCurrentScopeId() == id; }

bool ScopeManagerTestFixture::expectScopeCount(size_t count) { return m_scopeManager->getScopeCount() == count; }

bool ScopeManagerTestFixture::expectScopeId(size_t id)
{
    return m_scopeManager->searchById(id, 0, UINT64_MAX, nullptr);
}

bool ScopeManagerTestFixture::expectSubScopeCount(size_t count) { return m_scopeManager->getSubScopeCount() == count; }

bool ScopeManagerTestFixture::expectSymbol(SymbolType type, const std::string &name, const std::string &dataType)
{
    std::shared_ptr<Symbol> sym;

    if (!m_scopeManager->searchSymbolByName(name, 0, UINT64_MAX, &sym))
        return false;

    if (sym->getType() != type || (!dataType.empty() && sym->getSymbolDataType() == dataType))
        return false;

    return true;
}
