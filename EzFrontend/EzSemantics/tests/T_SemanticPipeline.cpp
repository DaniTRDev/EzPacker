/**
 * @file T_SemanticPipeline.cpp
 * @brief Integration tests for the full semantic pipeline:
 *        SymbolDefinitionVisitor → SymbolAndTypeResolverVisitor → TypeCheckVisitor.
 *
 * Tests verify that after the three passes the AST carries the expected
 * annotations (SymbolAnnotation, DataTypeAnnotation, ScopeAnnotation),
 * that type defaults are applied correctly, that unresolved symbols are
 * detected, and that break/continue outside a loop are rejected.
 */
#include <gtest/gtest.h>
#include <EzSemantics.h>

using ModParser = ModuleParser::ModuleParser;
using MemParser = MemoryOperandParser::MemoryOperandParser;

class SemanticPipelineTests : public ::testing::Test
{
  protected:
    size_t currentTestId;
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<BasicSemanticContext> ctx;
    std::shared_ptr<BasicParsingContext> parsingCtx;

    void SetUp() override
    {
        currentTestId = 0;
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

        beginTestErrorScope();
    }

    void TearDown() override { endTestErrorScope(); }

    std::shared_ptr<BasicParsingContext> makeParseCtx(const std::string &src)
    {
        ec->beginScope();
        {
            std::string srcName = std::format("test_{}", currentTestId++);
            size_t id = sm->addSourceContent(srcName, src);
            BasicTokenizer tok(ec, sm);
            tok.tokenizeBuffer(0, id);
            parsingCtx = std::make_shared<BasicParsingContext>(ec, sm, tok.getTokens());
        }
        ec->endScope(ErrorAction::Propagate);

        return parsingCtx;
    }

    Module *parseModule(const std::string &src)
    {
        AstNode *node = nullptr;
        ec->beginScope();
        {
            auto pctx = makeParseCtx(src);
            ModParser parser;
            node = parser.parse(pctx);

            if (!node || node->getType() != AstNodeType::Module)
            {
                node = nullptr;
            }
        }
        ec->endScope(ErrorAction::Propagate);
        return dynamic_cast<Module *>(node);
    }

    /// Run definition + resolution + type-check on `node`. Returns true if no fatal errors.
    bool runFullPipeline(AstNode *node)
    {
        ec->beginScope();
        {
            SymbolDefinitionVisitor def;
            def.setSemanticContext(ctx);

            if (!node->accept(&def))
            {
                ec->endScope(ErrorAction::Commit);
                return false;
            }

            if (ec->doesCurrentScopeHasFatalErrors())
            {
                ec->endScope(ErrorAction::Commit);
                return false;
            }

            SymbolAndTypeResolverVisitor resolver;
            resolver.setSemanticContext(ctx);

            if (!node->accept(&resolver))
            {
                ec->endScope(ErrorAction::Commit);
                return false;
            }

            if (ec->doesCurrentScopeHasFatalErrors())
            {
                ec->endScope(ErrorAction::Commit);
                return false;
            }

            TypeCheckVisitor checker;
            checker.setSemanticContext(ctx);
            if (!node->accept(&checker))
            {
                ec->endScope(ErrorAction::Commit);
                return false;
            }
        }

        bool res = ec->doesCurrentScopeHasFatalErrors();
        ec->endScope(ErrorAction::Commit);

        return !res;
    }

    void beginTestErrorScope() { ec->beginScope(); }

    void endTestErrorScope() { ec->endScope(ErrorAction::Propagate); }
};

// ─── SymbolAndTypeResolverVisitor ─────────────────────────────────────────────

TEST_F(SemanticPipelineTests, VariableUseResolvedToDeclaration)
{
    Module *mod = parseModule("void foo(i64 %x) { mov %x, 0; }");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runFullPipeline(mod));
}

TEST_F(SemanticPipelineTests, UnresolvedVariableEmitsError)
{
    Module *mod = parseModule("void foo() { mov %undeclared, 0; }");
    ASSERT_NE(mod, nullptr);
    // Definition pass succeeds; resolution should fail / emit error.
    SymbolDefinitionVisitor def;
    def.setSemanticContext(ctx);
    mod->accept(&def);

    SymbolAndTypeResolverVisitor resolver;
    resolver.setSemanticContext(ctx);
    bool resolveOk = mod->accept(&resolver);

    // Either the pass returns false OR the error collector has errors.
    if (resolveOk)
        EXPECT_TRUE(ec->doesCurrentScopeHasFatalErrors());
    else
        SUCCEED();
}

TEST_F(SemanticPipelineTests, IntegerImmediateGetsDefaultType)
{
    // Parse a bare immediate and run resolution.
    auto pctx = makeParseCtx("42");
    ImmediateParser::ImmediateParser immParser;
    AstNode *immNode = immParser.parse(pctx);
    ASSERT_NE(immNode, nullptr);
    ASSERT_EQ(immNode->getType(), AstNodeType::Immediate);

    SymbolAndTypeResolverVisitor resolver;
    resolver.setSemanticContext(ctx);
    immNode->accept(&resolver);

    // Should have a DataTypeAnnotation with "i64" (the default).
    auto *annot = immNode->getAnnotation<DataTypeAnnotation>();
    ASSERT_NE(annot, nullptr);
    ASSERT_NE(annot->getDataType(), nullptr);
    EXPECT_EQ(annot->getDataType()->getTypeName(), "i64");
}

TEST_F(SemanticPipelineTests, ExplicitTypeOnImmediateIsUsed)
{
    auto pctx = makeParseCtx("i16 0xFF");
    ImmediateParser::ImmediateParser immParser;
    AstNode *immNode = immParser.parse(pctx);
    ASSERT_NE(immNode, nullptr);

    SymbolAndTypeResolverVisitor resolver;
    resolver.setSemanticContext(ctx);
    immNode->accept(&resolver);

    auto *annot = immNode->getAnnotation<DataTypeAnnotation>();
    ASSERT_NE(annot, nullptr);
    ASSERT_NE(annot->getDataType(), nullptr);
    EXPECT_EQ(annot->getDataType()->getTypeName(), "i16");
}

TEST_F(SemanticPipelineTests, MemoryOperandGetsTypeAnnotation)
{
    auto pctx = makeParseCtx("i32 (%base+4)");
    MemParser memParser;
    AstNode *memNode = memParser.parse(pctx);
    ASSERT_NE(memNode, nullptr);

    SymbolAndTypeResolverVisitor resolver;
    resolver.setSemanticContext(ctx);
    memNode->accept(&resolver);

    auto *annot = memNode->getAnnotation<DataTypeAnnotation>();
    ASSERT_NE(annot, nullptr);
    ASSERT_NE(annot->getDataType(), nullptr);
    EXPECT_EQ(annot->getDataType()->getTypeName(), "i32");
}

// ─── TypeCheckVisitor ─────────────────────────────────────────────────────────

TEST_F(SemanticPipelineTests, WellTypedModulePassesCheck)
{
    Module *mod = parseModule("void foo(i64 %a) { nop; }");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runFullPipeline(mod));
}

TEST_F(SemanticPipelineTests, BreakOutsideLoopAndSwitchEmitsError)
{
    Module *mod = parseModule("void foo() { break; }");
    ASSERT_NE(mod, nullptr);

    SymbolDefinitionVisitor def;
    def.setSemanticContext(ctx);
    mod->accept(&def);

    SymbolAndTypeResolverVisitor resolver;
    resolver.setSemanticContext(ctx);
    mod->accept(&resolver);

    TypeCheckVisitor checker;
    checker.setSemanticContext(ctx);
    bool ok = mod->accept(&checker);

    if (ok)
        EXPECT_TRUE(ec->doesCurrentScopeHasFatalErrors());
    else
        SUCCEED();
}

TEST_F(SemanticPipelineTests, ContinueOutsideLoopEmitsError)
{
    Module *mod = parseModule("void foo() { continue; }");
    ASSERT_NE(mod, nullptr);

    SymbolDefinitionVisitor def;
    def.setSemanticContext(ctx);
    mod->accept(&def);

    SymbolAndTypeResolverVisitor resolver;
    resolver.setSemanticContext(ctx);
    mod->accept(&resolver);

    TypeCheckVisitor checker;
    checker.setSemanticContext(ctx);
    bool ok = mod->accept(&checker);

    if (ok)
        EXPECT_TRUE(ec->doesCurrentScopeHasFatalErrors());
    else
        SUCCEED();
}

TEST_F(SemanticPipelineTests, BreakInsideWhileIsLegal)
{
    Module *mod = parseModule("void foo(i64 %i) {"
                              "  while (%i GT 0) { break; }"
                              "}");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runFullPipeline(mod));
}

TEST_F(SemanticPipelineTests, ContinueInsideWhileIsLegal)
{
    Module *mod = parseModule("void foo(i64 %i) {"
                              "  while (%i GT 0) { continue; }"
                              "}");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runFullPipeline(mod));
}

TEST_F(SemanticPipelineTests, BreakInsideSwitchIsLegal)
{
    Module *mod = parseModule("void foo(i64 %x) {"
                              "  switch (%x) { case 1: { break; } }"
                              "}");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runFullPipeline(mod));
}

TEST_F(SemanticPipelineTests, NestedLoopBreakIsLegal)
{
    Module *mod = parseModule("void foo(i64 %i, i64 %j) {"
                              "  while (%i GT 0) {"
                              "    while (%j GT 0) { break; }"
                              "  }"
                              "}");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runFullPipeline(mod));
}

// ─── Additional semantic pipeline tests ──────────────────────────────────────

TEST_F(SemanticPipelineTests, ContinueInsideForIsLegal)
{
    Module *mod = parseModule("void foo() {"
                              "  for ({create i64 %i; mov %i, 0;} (%i LT 10) {add %i, 1;}) { continue; }"
                              "}");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runFullPipeline(mod));
}

TEST_F(SemanticPipelineTests, BreakInsideForIsLegal)
{
    Module *mod = parseModule("void foo() {"
                              "  for ({create i64 %i; mov %i, 0;} (%i LT 10) {add %i, 1;}) { break; }"
                              "}");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runFullPipeline(mod));
}

TEST_F(SemanticPipelineTests, ContinueInsideSwitchEmitsError)
{
    // continue is only valid inside loops, not switches
    Module *mod = parseModule("void foo(i64 %x) {"
                              "  switch (%x) { case 1: { continue; } }"
                              "}");
    ASSERT_NE(mod, nullptr);

    SymbolDefinitionVisitor def;
    def.setSemanticContext(ctx);
    mod->accept(&def);

    SymbolAndTypeResolverVisitor resolver;
    resolver.setSemanticContext(ctx);
    mod->accept(&resolver);

    TypeCheckVisitor checker;
    checker.setSemanticContext(ctx);
    bool ok = mod->accept(&checker);

    if (ok)
        EXPECT_TRUE(ec->doesCurrentScopeHasFatalErrors());
    else
        SUCCEED();
}

TEST_F(SemanticPipelineTests, VariableInWhileConditionResolved)
{
    Module *mod = parseModule("void foo(i64 %x) {"
                              "  while (%x GT 0) { nop; }"
                              "}");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runFullPipeline(mod));
}

TEST_F(SemanticPipelineTests, VariableInIfConditionResolved)
{
    Module *mod = parseModule("void foo(i64 %a, i64 %b) {"
                              "  if (%a EQ %b) { nop; }"
                              "}");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runFullPipeline(mod));
}

TEST_F(SemanticPipelineTests, VariableInSwitchResolved)
{
    Module *mod = parseModule("void foo(i64 %x) {"
                              "  switch (%x) { case 1: { nop; } }"
                              "}");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runFullPipeline(mod));
}

TEST_F(SemanticPipelineTests, ModuleWithMultipleParametersPassesCheck)
{
    Module *mod = parseModule("void foo(i64 %a, i32 %b, i8 %c) { nop; }");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runFullPipeline(mod));
}

TEST_F(SemanticPipelineTests, EmptyModulePassesCheck)
{
    Module *mod = parseModule("void empty() {}");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runFullPipeline(mod));
}

TEST_F(SemanticPipelineTests, NestedWhileWithContinueIsLegal)
{
    Module *mod = parseModule("void foo(i64 %i, i64 %j) {"
                              "  while (%i GT 0) {"
                              "    while (%j GT 0) { continue; }"
                              "  }"
                              "}");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runFullPipeline(mod));
}

TEST_F(SemanticPipelineTests, BreakInsideNestedSwitchInLoopIsLegal)
{
    Module *mod = parseModule("void foo(i64 %x) {"
                              "  while (%x GT 0) {"
                              "    switch (%x) { case 1: { break; } }"
                              "  }"
                              "}");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runFullPipeline(mod));
}

TEST_F(SemanticPipelineTests, LocalVariableUsedAfterCreateResolved)
{
    Module *mod = parseModule("void foo() { create i64 %v; mov %v, 0; }");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runFullPipeline(mod));
}

TEST_F(SemanticPipelineTests, VariableInvalidVoidType)
{
    Module *mod = parseModule("void mod1() { create void %v; }");
    ASSERT_NE(mod, nullptr);
    EXPECT_FALSE(runFullPipeline(mod));
}

TEST_F(SemanticPipelineTests, TypeMismatchError)
{
    Module *mod = parseModule("void foo() { create i64 %v; mov %v, 3.14; }");
    ASSERT_NE(mod, nullptr);
    EXPECT_FALSE(runFullPipeline(mod));
}

TEST_F(SemanticPipelineTests, FunctionCallResolved)
{
    Module *mod = parseModule("void callee() {} void caller() { call callee(); }");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runFullPipeline(mod));
}

TEST_F(SemanticPipelineTests, FunctionCallWithArgumentsResolved)
{
    Module *mod = parseModule("void callee(i64 %a, i64 %b) { call %callee(1, 2); }");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runFullPipeline(mod));
}

TEST_F(SemanticPipelineTests, FunctionCallWithIncorrectArgumentCountFails)
{
    Module *mod = parseModule("void callee(i64 %a) { call %callee(1, 2); }");
    ASSERT_NE(mod, nullptr);
    EXPECT_FALSE(runFullPipeline(mod));
}

TEST_F(SemanticPipelineTests, FunctionCallWithDoubleToIntArg)
{
    Module *mod = parseModule("void callee(i64 %a) {call %callee(3.14); }");
    ASSERT_NE(mod, nullptr);
    EXPECT_TRUE(runFullPipeline(mod));
}
