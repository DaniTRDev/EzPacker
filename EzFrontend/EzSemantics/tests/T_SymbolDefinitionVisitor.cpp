/**
 * @file T_SymbolDefinitionVisitor.cpp
 * @brief Unit tests for SymbolDefinitionVisitor.
 *
 * Covers: module symbol creation, parameter definition, label symbol,
 * global variable, local variable via 'create', redefinition error,
 * scope isolation between sibling if-branches.
 */
#include <gtest/gtest.h>
#include <EzSemantics.h>

using ModParser = ModuleParser::ModuleParser;

class SymbolDefinitionVisitorTests : public ::testing::Test
{
  protected:
    size_t currentTestId = 0;
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<BasicSemanticContext> ctx;
    std::shared_ptr<BasicParsingContext> parsingCtx;

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

    void TearDown() override { ec->endScope(ErrorAction::Commit); }

    /// Build a parsing context from raw source text.
    std::shared_ptr<BasicParsingContext> makeParseCtx(const std::string &src)
    {
        size_t id = sm->addSourceContent(std::format("test_{}", currentTestId++), src);
        BasicTokenizer tok(ec, sm);
        tok.tokenizeBuffer(0, id);
        parsingCtx = std::make_shared<BasicParsingContext>(ec, sm, tok.getTokens());
        return parsingCtx;
    }

    /// Parse a full module using ModuleParser and return the Module node.
    Module *parseModule(const std::string &src)
    {
        auto pctx = makeParseCtx(src);
        ModParser parser;
        AstNode *node = parser.parse(pctx);
        if (!node || node->getType() != AstNodeType::Module)
            return nullptr;
        return dynamic_cast<Module *>(node);
    }

    /// Run SymbolDefinitionVisitor on the given node.
    bool runDefPass(AstNode *node) const
    {
        SymbolDefinitionVisitor vis;
        vis.setSemanticContext(ctx);
        return node->accept(&vis);
    }
};

// ─── Module-level tests ───────────────────────────────────────────────────────

TEST_F(SymbolDefinitionVisitorTests, ModuleSymbolCreated)
{
    Module *mod = parseModule("void myModule() {}");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runDefPass(mod));

    Symbol *sym = nullptr;
    EXPECT_TRUE(ctx->resolveSymbolInScope("myModule", &sym, false));
    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->getType(), SymbolType::Module);
}

TEST_F(SymbolDefinitionVisitorTests, ModuleParametersDefinedInModuleScope)
{
    Module *mod = parseModule("i64 add(i64 %a, i64 %b) { nop; }");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runDefPass(mod));

    // Module uses ScopedSymbolAnnotation (which extends SymbolAnnotation and
    // owns a child scope).
    auto *annot = mod->getAnnotation<ScopedSymbolAnnotation>();
    ASSERT_NE(annot, nullptr);
    Scope *modScope = annot->getOwnedScope();
    ASSERT_NE(modScope, nullptr);
    Symbol *a = nullptr, *b = nullptr;
    EXPECT_TRUE(modScope->resolve("a", &a, false));
    EXPECT_TRUE(modScope->resolve("b", &b, false));
    EXPECT_EQ(a->getType(), SymbolType::LocalVariable);
}

TEST_F(SymbolDefinitionVisitorTests, LabelSymbolCreated)
{
    Module *mod = parseModule("void foo() { entry: { nop; } }");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runDefPass(mod));

    // The module's owned scope (via ScopedSymbolAnnotation) holds the label.
    auto *annot = mod->getAnnotation<ScopedSymbolAnnotation>();
    ASSERT_NE(annot, nullptr);
    Symbol *labelSym = nullptr;
    EXPECT_TRUE(annot->getOwnedScope()->resolve("entry", &labelSym, false));
    ASSERT_NE(labelSym, nullptr);
    EXPECT_EQ(labelSym->getType(), SymbolType::Label);
}

TEST_F(SymbolDefinitionVisitorTests, LocalVariableViaCreateInstruction)
{
    Module *mod = parseModule("void foo() { create i64 %localVar; }");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runDefPass(mod));

    auto *annot = mod->getAnnotation<ScopedSymbolAnnotation>();
    ASSERT_NE(annot, nullptr);
    Symbol *sym = nullptr;
    EXPECT_TRUE(annot->getOwnedScope()->resolve("localVar", &sym, false));
    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->getType(), SymbolType::LocalVariable);
}

TEST_F(SymbolDefinitionVisitorTests, GlobalVariableAtModuleTopLevel)
{
    // A variable declared outside a module is global.
    auto pctx = makeParseCtx("i64 %globalVar: {0}");
    VariableParser vp;
    AstNode *var = vp.parse(pctx);
    ASSERT_NE(var, nullptr);
    EXPECT_TRUE(runDefPass(var));

    Symbol *sym = nullptr;
    EXPECT_TRUE(ctx->resolveSymbolInScope("globalVar", &sym, false));
    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->getType(), SymbolType::GlobalVariable);
}

TEST_F(SymbolDefinitionVisitorTests, RedefinitionEmitsError)
{
    // Two modules with the same name in the same scope.
    Module *mod1 = parseModule("void dup() {}");
    Module *mod2 = parseModule("void dup() {}");
    ASSERT_NE(mod1, nullptr);
    ASSERT_NE(mod2, nullptr);

    EXPECT_TRUE(runDefPass(mod1));
    // The second definition of 'dup' should fail.
    EXPECT_FALSE(runDefPass(mod2));
}

TEST_F(SymbolDefinitionVisitorTests, SiblingIfBranchesHaveSeparateScopes)
{
    Module *mod = parseModule("void foo(i64 %x) {"
                              "  if (%x EQ 0) { create i64 %a; }"
                              "  else { create i64 %b; }"
                              "}");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runDefPass(mod));
    // No fatal errors expected — 'a' and 'b' are in separate scopes.
    EXPECT_FALSE(ec->doesCurrentScopeHasFatalErrors());
}

// ─── Additional SymbolDefinitionVisitor tests ─────────────────────────────────

TEST_F(SymbolDefinitionVisitorTests, WhileBodyCreatesScope)
{
    Module *mod = parseModule("void foo(i64 %i) {"
                              "  while (%i GT 0) { create i64 %loopVar; }"
                              "}");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runDefPass(mod));
    EXPECT_FALSE(ec->doesCurrentScopeHasFatalErrors());
}

TEST_F(SymbolDefinitionVisitorTests, ForBodyCreatesScope)
{
    Module *mod = parseModule("void foo() {"
                              "  for ({create i64 %i; mov %i, 0;} (%i LT 10) {add %i, 1;}) { create i64 %tmp; }"
                              "}");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runDefPass(mod));
    EXPECT_FALSE(ec->doesCurrentScopeHasFatalErrors());
}

TEST_F(SymbolDefinitionVisitorTests, MultipleLabelSymbolsCreated)
{
    Module *mod = parseModule("void foo() { a: { nop; } b: { nop; } }");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runDefPass(mod));

    auto *annot = mod->getAnnotation<ScopedSymbolAnnotation>();
    ASSERT_NE(annot, nullptr);
    Symbol *aSym = nullptr, *bSym = nullptr;
    EXPECT_TRUE(annot->getOwnedScope()->resolve("a", &aSym, false));
    EXPECT_TRUE(annot->getOwnedScope()->resolve("b", &bSym, false));
    ASSERT_NE(aSym, nullptr);
    ASSERT_NE(bSym, nullptr);
    EXPECT_EQ(aSym->getType(), SymbolType::Label);
    EXPECT_EQ(bSym->getType(), SymbolType::Label);
    EXPECT_NE(aSym->getId(), bSym->getId());
}

TEST_F(SymbolDefinitionVisitorTests, NestedIfScopesIsolateVariables)
{
    Module *mod = parseModule("void foo(i64 %x) {"
                              "  if (%x GT 0) {"
                              "    if (%x GT 10) { create i64 %inner; }"
                              "  }"
                              "}");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runDefPass(mod));
    EXPECT_FALSE(ec->doesCurrentScopeHasFatalErrors());
}

TEST_F(SymbolDefinitionVisitorTests, SwitchBodyCreatesScope)
{
    Module *mod = parseModule("void foo(i64 %x) {"
                              "  switch (%x) { case 1: { create i64 %caseVar; } }"
                              "}");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runDefPass(mod));
    EXPECT_FALSE(ec->doesCurrentScopeHasFatalErrors());
}

TEST_F(SymbolDefinitionVisitorTests, ModuleReturnTypeIsStoredInHeader)
{
    Module *mod = parseModule("i64 compute() { nop; }");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runDefPass(mod));
    EXPECT_EQ(mod->getHeader()->getReturnTypeName(), "i64");
}

TEST_F(SymbolDefinitionVisitorTests, DuplicateParameterNamesEmitError)
{
    Module *mod = parseModule("void foo(i64 %a, i64 %a) { nop; }");
    ASSERT_NE(mod, nullptr);
    EXPECT_FALSE(runDefPass(mod));
    EXPECT_TRUE(ec->doesCurrentScopeHasFatalErrors());
}
