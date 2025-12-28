#ifndef EZPACKER_SCOPEMANAGERTESTFIXTURE_H
#define EZPACKER_SCOPEMANAGERTESTFIXTURE_H

#include "EzSemantics.h"
#include <gtest/gtest.h>

class ScopeManagerTestFixture : public ::testing::Test
{
  public:
    /**
     * Creates the test fixture by instantiating the scope manager.
     */
    void SetUp() override;

    /**
     * Destroys the instance of the scope manager.
     */
    void TearDown() override;

    /**
     * Expects the current scope ID to match the one given.
     * @param id
     * @return bool
     */
    bool expectCurrentScopeId(size_t id);

    /**
     * Returns true if current scope count matches the one given.
     * @param count
     * @return bool
     */
    bool expectScopeCount(size_t count);

    /**
     * Returns true if current scope's ID matches the one given.
     * @param id
     * @return bool
     */
    bool expectScopeId(size_t id);

    /**
     * Returns true the number of scopes inside the current one matches 'count'.
     * @param count
     * @return bool
     */
    bool expectSubScopeCount(size_t count);

    /**
     * Returns true if there's a symbol in the current scope that matches given description. If dataType is not set, it
     * won't be checked.
     * @param type
     * @param name
     * @param dataType
     * @return bool
     */
    bool expectSymbol(SymbolType type, const std::string &name, const std::string &dataType = "");

  public:
    std::shared_ptr<ScopeManager> m_scopeManager;
};

// Does the given scopeId exist?
inline constexpr auto TEST_SCOPE_IMPL = [](size_t scopeId, ScopeManagerTestFixture *fixture)
{ EXPECT_TRUE(fixture->expectScopeId(scopeId)); };

// Is the current scope id == given id?
inline constexpr auto TEST_CURRENT_SCOPE_IMPL = [](size_t scopeId, ScopeManagerTestFixture *fixture)
{ EXPECT_TRUE(fixture->expectCurrentScopeId(scopeId)); };

inline constexpr auto TEST_CURRENT_SCOPE_SYMBOL_IMPL = [](size_t scopeId,
                                                          SymbolType type,
                                                          const std::string &symbolName,
                                                          const std::string &symbolDataType,
                                                          ScopeManagerTestFixture *fixture)
{
    TEST_CURRENT_SCOPE_IMPL(scopeId, fixture);
    fixture->expectSymbol(type, symbolName, symbolDataType);
};

#define TEST_SCOPE(scopeId) TEST_SCOPE_IMPL(scopeId, this);
#define TEST_CURRENT_SCOPE(scopeId) TEST_CURRENT_SCOPE_IMPL(scopeId, this);

#define TEST_CURRENT_SCOPE_SYMBOL(scopeId, type, symbolName, symbolDataType)                                           \
    TEST_CURRENT_SCOPE_SYMBOL_IMPL(scopeId, type, symbolName, symbolDataType, this);

#endif // EZPACKER_SCOPEMANAGERTESTFIXTURE_H
