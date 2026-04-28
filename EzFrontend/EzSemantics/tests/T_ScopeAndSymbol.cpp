/**
 * @file T_ScopeAndSymbol.cpp
 * @brief Unit tests for Scope, Symbol, and BasicSemanticContext symbol-table
 *        operations.
 *
 * Covers: define + resolve, parent-chain lookup, shadowing, duplicate
 * rejection, mergeSymbols, isCurrentScopeGlobalScope, loop/switch tracking.
 */
#include <gtest/gtest.h>
#include <filesystem>
#include <EzSemantics.h>

class ScopeAndSymbolTests : public ::testing::Test
{
  protected:
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<BasicSemanticContext> ctx;

    void SetUp() override
    {
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        ctx = std::make_shared<BasicSemanticContext>(ec, sm);

        ec->addSubscriber(
                [](void *userParam, const std::shared_ptr<Error> &error)
                {
                    SourceManager *sourceManager = (SourceManager *)userParam;
                    if (error->m_sourceRef.m_valid)
                    {
                        g_logger->pushLog(LogMessage("[{} - {}] {}:{}:{} {} \n\t {}",
                                                     error->m_sender,
                                                     error->m_timeStamp,
                                                     sourceManager->getSourceName(error->m_sourceRef.m_sourceFileId),
                                                     error->m_sourceRef.m_line,
                                                     error->m_sourceRef.m_col,
                                                     error->m_message,
                                                     sourceManager->getReferenceContent(error->m_sourceRef)));
                    }
                    else
                    {
                        g_logger->pushLog(
                                LogMessage("[{}]{} {}", error->m_sender, error->m_timeStamp, error->m_message));
                    }
                },
                sm.get());

        ec->beginScope();
    }

    void TearDown() override { ec->endScope(ErrorAction::Discard); }
};

// ─── TypeTable ───────────────────────────────────────────────────────────────

TEST_F(ScopeAndSymbolTests, BuiltinIntegerTypesExist)
{
    for (auto name : { "i8", "i16", "i32", "i64", "i128", "i256", "i512" })
        EXPECT_TRUE(ctx->getTypeTable()->doesTypeExists(name)) << name;
}

TEST_F(ScopeAndSymbolTests, FloatingPointTypesExist)
{
    EXPECT_TRUE(ctx->getTypeTable()->doesTypeExists("double"));
}

TEST_F(ScopeAndSymbolTests, VoidAndStringExist)
{
    EXPECT_TRUE(ctx->getTypeTable()->doesTypeExists("void"));
    EXPECT_TRUE(ctx->getTypeTable()->doesTypeExists("string"));
}

TEST_F(ScopeAndSymbolTests, UnknownTypeDoesNotExist)
{
    EXPECT_FALSE(ctx->getTypeTable()->doesTypeExists("u640"));
    EXPECT_FALSE(ctx->getTypeTable()->doesTypeExists(""));
}

TEST_F(ScopeAndSymbolTests, GetTypeReturnsNonNullForKnown)
{
    auto t = ctx->getTypeTable()->getType("i64");
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->getTypeName(), "i64");
}

TEST_F(ScopeAndSymbolTests, GetTypeReturnsNullForUnknown)
{
    EXPECT_EQ(ctx->getTypeTable()->getType("unknown_type"), nullptr);
}

TEST_F(ScopeAndSymbolTests, DefaultTypeIsI64)
{
    auto def = ctx->getTypeTable()->getDefaultType();
    ASSERT_NE(def, nullptr);
    EXPECT_EQ(def->getTypeName(), "i64");
}

// ─── Scope ───────────────────────────────────────────────────────────────────

TEST_F(ScopeAndSymbolTests, DefineAndResolveInSameScope)
{
    auto sym = std::make_shared<Symbol>(nullptr, nullptr, SymbolType::LocalVariable, "x");
    Scope scope(nullptr, "test");
    EXPECT_TRUE(scope.define(sym.get(), "x"));
    Symbol *out = nullptr;
    EXPECT_TRUE(scope.resolve("x", &out, false));
    EXPECT_EQ(out, sym.get());
}

TEST_F(ScopeAndSymbolTests, ResolveUnknownReturnsFalse)
{
    Scope scope(nullptr, "test");
    Symbol *out = nullptr;
    EXPECT_FALSE(scope.resolve("notDefined", &out, false));
    EXPECT_EQ(out, nullptr);
}

TEST_F(ScopeAndSymbolTests, DuplicateDefineReturnsFalse)
{
    auto sym1 = std::make_shared<Symbol>(nullptr, nullptr, SymbolType::LocalVariable, "x");
    auto sym2 = std::make_shared<Symbol>(nullptr, nullptr, SymbolType::LocalVariable, "x");
    Scope scope(nullptr, "test");
    EXPECT_TRUE(scope.define(sym1.get(), "x"));
    EXPECT_FALSE(scope.define(sym2.get(), "x"));
}

TEST_F(ScopeAndSymbolTests, ParentChainLookupSucceeds)
{
    auto sym = std::make_shared<Symbol>(nullptr, nullptr, SymbolType::LocalVariable, "parent_var");
    Scope parent(nullptr, "parent");
    parent.define(sym.get(), "parent_var");

    Scope child(&parent, "child");
    Symbol *out = nullptr;
    EXPECT_TRUE(child.resolve("parent_var", &out, true));
    EXPECT_EQ(out, sym.get());
}

TEST_F(ScopeAndSymbolTests, ParentChainLookupDisabledDoesNotFindParentSymbol)
{
    auto sym = std::make_shared<Symbol>(nullptr, nullptr, SymbolType::LocalVariable, "parent_var");
    Scope parent(nullptr, "parent");
    parent.define(sym.get(), "parent_var");

    Scope child(&parent, "child");
    Symbol *out = nullptr;
    EXPECT_FALSE(child.resolve("parent_var", &out, false));
}

TEST_F(ScopeAndSymbolTests, ShadowingAllowed)
{
    auto parentSym = std::make_shared<Symbol>(nullptr, nullptr, SymbolType::LocalVariable, "x");
    auto childSym = std::make_shared<Symbol>(nullptr, nullptr, SymbolType::LocalVariable, "x");

    Scope parent(nullptr, "parent");
    parent.define(parentSym.get(), "x");

    Scope child(&parent, "child");
    EXPECT_TRUE(child.define(childSym.get(), "x")); // shadowing is allowed

    Symbol *out = nullptr;
    child.resolve("x", &out, false);
    EXPECT_EQ(out, childSym.get()); // child wins
}

TEST_F(ScopeAndSymbolTests, MergeSymbolsSucceeds)
{
    auto sym = std::make_shared<Symbol>(nullptr, nullptr, SymbolType::LocalVariable, "z");
    std::map<std::string_view, Symbol *> incoming = { { "z", sym.get() } };

    Scope scope(nullptr, "test");
    Symbol *errSym = nullptr;
    EXPECT_TRUE(scope.mergeSymbols(incoming, &errSym));

    Symbol *out = nullptr;
    EXPECT_TRUE(scope.resolve("z", &out, false));
    EXPECT_EQ(out, sym.get());
}

TEST_F(ScopeAndSymbolTests, MergeSymbolsConflictReturnsFalse)
{
    auto sym1 = std::make_shared<Symbol>(nullptr, nullptr, SymbolType::LocalVariable, "z");
    auto sym2 = std::make_shared<Symbol>(nullptr, nullptr, SymbolType::LocalVariable, "z");

    Scope scope(nullptr, "test");
    scope.define(sym1.get(), "z");

    std::map<std::string_view, Symbol *> incoming = { { "z", sym2.get() } };
    Symbol *errSym = nullptr;
    EXPECT_FALSE(scope.mergeSymbols(incoming, &errSym));
}

TEST_F(ScopeAndSymbolTests, GetParentReturnsExpected)
{
    Scope parent(nullptr, "parent");
    Scope child(&parent, "child");
    EXPECT_EQ(child.getParent(), &parent);
}

TEST_F(ScopeAndSymbolTests, SetParentUpdatesPointer)
{
    Scope a(nullptr, "a");
    Scope b(nullptr, "b");
    b.setParent(&a);
    EXPECT_EQ(b.getParent(), &a);
}

// ─── BasicSemanticContext ─────────────────────────────────────────────────────

TEST_F(ScopeAndSymbolTests, GlobalScopeIsCreatedOnConstruction)
{
    EXPECT_TRUE(ctx->isCurrentScopeGlobalScope());
    EXPECT_NE(ctx->getCurrentScope(), nullptr);
    EXPECT_NE(ctx->getGlobalScope(), nullptr);
}

TEST_F(ScopeAndSymbolTests, CreateSymbolSucceeds)
{
    auto type = ctx->getTypeTable()->getType("i64");
    Symbol *sym = nullptr;
    EXPECT_TRUE(ctx->createSymbol(nullptr, SymbolType::LocalVariable, &sym, type.get(), "myVar"));
    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->getName(), "myVar");
    EXPECT_EQ(sym->getType(), SymbolType::LocalVariable);
}

TEST_F(ScopeAndSymbolTests, CreateDuplicateSymbolFails)
{
    Symbol *s1 = nullptr, *s2 = nullptr;
    ctx->createSymbol(nullptr, SymbolType::LocalVariable, &s1, nullptr, "dup");
    EXPECT_FALSE(ctx->createSymbol(nullptr, SymbolType::LocalVariable, &s2, nullptr, "dup"));
}

TEST_F(ScopeAndSymbolTests, SymbolIdIsUnique)
{
    Symbol *s1 = nullptr, *s2 = nullptr;
    ctx->createSymbol(nullptr, SymbolType::LocalVariable, &s1, nullptr, "a");
    ctx->createSymbol(nullptr, SymbolType::LocalVariable, &s2, nullptr, "b");
    ASSERT_NE(s1, nullptr);
    ASSERT_NE(s2, nullptr);
    EXPECT_NE(s1->getId(), s2->getId());
}

TEST_F(ScopeAndSymbolTests, ResolveSymbolSucceeds)
{
    Symbol *created = nullptr;
    ctx->createSymbol(nullptr, SymbolType::LocalVariable, &created, nullptr, "v");

    Symbol *resolved = nullptr;
    EXPECT_TRUE(ctx->resolveSymbolInScope("v", &resolved, false));
    EXPECT_EQ(resolved, created);
}

TEST_F(ScopeAndSymbolTests, IsNotInsideLoopByDefault) { EXPECT_FALSE(ctx->isContextInsideLoop()); }

TEST_F(ScopeAndSymbolTests, IsNotInsideSwitchByDefault) { EXPECT_FALSE(ctx->isContextInsideSwitch()); }

// ─── Additional scope and context tests ───────────────────────────────────────

TEST_F(ScopeAndSymbolTests, BeginAndEndScopeChangesCurrentScope)
{
    Scope *global = ctx->getCurrentScope();
    ctx->beginScope("child");
    Scope *child = ctx->getCurrentScope();
    EXPECT_NE(child, global);
    EXPECT_EQ(child->getParent(), global);
    ctx->endScope();
    EXPECT_EQ(ctx->getCurrentScope(), global);
}

TEST_F(ScopeAndSymbolTests, EnterAndExitLoopTracksNesting)
{
    EXPECT_FALSE(ctx->isContextInsideLoop());
    ctx->enterLoop();
    EXPECT_TRUE(ctx->isContextInsideLoop());
    ctx->enterLoop(); // nested
    EXPECT_TRUE(ctx->isContextInsideLoop());
    ctx->exitLoop();
    EXPECT_TRUE(ctx->isContextInsideLoop()); // still in outer loop
    ctx->exitLoop();
    EXPECT_FALSE(ctx->isContextInsideLoop());
}

TEST_F(ScopeAndSymbolTests, EnterAndExitSwitchTracksNesting)
{
    EXPECT_FALSE(ctx->isContextInsideSwitch());
    ctx->enterSwitch();
    EXPECT_TRUE(ctx->isContextInsideSwitch());
    ctx->exitSwitch();
    EXPECT_FALSE(ctx->isContextInsideSwitch());
}

TEST_F(ScopeAndSymbolTests, NestedScopeSymbolNotVisibleAfterEnd)
{
    ctx->beginScope("inner");
    Symbol *sym = nullptr;
    ctx->createSymbol(nullptr, SymbolType::LocalVariable, &sym, nullptr, "localOnly");
    ctx->endScope();

    Symbol *out = nullptr;
    EXPECT_FALSE(ctx->resolveSymbolInScope("localOnly", &out, false));
}

TEST_F(ScopeAndSymbolTests, AnnotationPoolAccessible) { EXPECT_NE(ctx->getAnnotPool(), nullptr); }

TEST_F(ScopeAndSymbolTests, SymbolPoolAccessible) { EXPECT_NE(ctx->getSymbolPool(), nullptr); }

TEST_F(ScopeAndSymbolTests, CreateSymbolWithType)
{
    auto type = ctx->getTypeTable()->getType("i32");
    Symbol *sym = nullptr;
    EXPECT_TRUE(ctx->createSymbol(nullptr, SymbolType::LocalVariable, &sym, type.get(), "typed"));
    ASSERT_NE(sym, nullptr);
    ASSERT_NE(sym->getSymbolDataType(), nullptr);
    EXPECT_EQ(sym->getSymbolDataType()->getTypeName(), "i32");
}

TEST_F(ScopeAndSymbolTests, SharedGlobalScopeAcrossContexts)
{
    auto globalScope = ctx->getGlobalScope();
    auto ctx2 = std::make_shared<BasicSemanticContext>(ec, sm, globalScope);

    Symbol *sym = nullptr;
    ctx->createSymbol(nullptr, SymbolType::Module, &sym, nullptr, "sharedMod");

    Symbol *out = nullptr;
    EXPECT_TRUE(ctx2->resolveSymbolInScope("sharedMod", &out, false));
    EXPECT_EQ(out, sym);
}

TEST_F(ScopeAndSymbolTests, GetSymbolsReturnsDefinedSymbols)
{
    auto sym1 = std::make_shared<Symbol>(nullptr, nullptr, SymbolType::LocalVariable, "a");
    auto sym2 = std::make_shared<Symbol>(nullptr, nullptr, SymbolType::LocalVariable, "b");
    Scope scope(nullptr, "test");
    scope.define(sym1.get(), "a");
    scope.define(sym2.get(), "b");
    EXPECT_EQ(scope.getSymbols().size(), 2u);
    EXPECT_NE(scope.getSymbols().find("a"), scope.getSymbols().end());
    EXPECT_NE(scope.getSymbols().find("b"), scope.getSymbols().end());
}

TEST_F(ScopeAndSymbolTests, AllBuiltinIntegerSizesHaveCorrectNames)
{
    struct TC
    {
        const char *name;
    };
    TC cases[] = { { "i8" }, { "i16" }, { "i32" }, { "i64" }, { "i128" }, { "i256" }, { "i512" } };
    for (auto &tc : cases)
    {
        auto t = ctx->getTypeTable()->getType(tc.name);
        ASSERT_NE(t, nullptr) << tc.name;
        EXPECT_EQ(t->getTypeName(), tc.name) << tc.name;
    }
}

TEST_F(ScopeAndSymbolTests, ShadowingInNestedScope)
{
    Symbol *globalSym = nullptr;
    ctx->createSymbol(nullptr, SymbolType::LocalVariable, &globalSym, nullptr, "x");

    ctx->beginScope("inner");
    Symbol *localSym = nullptr;
    ctx->createSymbol(nullptr, SymbolType::LocalVariable, &localSym, nullptr, "x");

    Symbol *resolved = nullptr;
    EXPECT_TRUE(ctx->resolveSymbolInScope("x", &resolved, false));
    EXPECT_EQ(resolved, localSym);
    EXPECT_NE(resolved, globalSym);

    ctx->endScope();

    EXPECT_TRUE(ctx->resolveSymbolInScope("x", &resolved, false));
    EXPECT_EQ(resolved, globalSym);
}
