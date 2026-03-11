#include <gtest/gtest.h>
#include <EzSemantics.h>
#include <EzMir.h>

using ModParser = ModuleParser::ModuleParser;

class AstLowererTests : public ::testing::Test
{
  protected:
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<BasicSemanticContext> semanticCtx;
    std::shared_ptr<BasicParsingContext> parsingCtx;

    std::shared_ptr<MirEmitterContext> mirCtx;
    std::shared_ptr<MirEmitter> mirEmitter;
    std::shared_ptr<MirGlobalDataEmitter> globalDataEmitter;
    std::shared_ptr<LoweringContext> loweringCtx;

    void SetUp() override
    {
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        semanticCtx = std::make_shared<BasicSemanticContext>(ec, sm);

        mirCtx = std::make_shared<MirEmitterContext>(ec, sm);
        mirEmitter = std::make_shared<MirEmitter>(mirCtx.get());
        globalDataEmitter = std::make_shared<MirGlobalDataEmitter>(mirCtx.get());
        loweringCtx = std::make_shared<LoweringContext>(semanticCtx, mirEmitter, mirCtx, globalDataEmitter);

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
    }

    void TearDown() override {}

    std::shared_ptr<BasicParsingContext> makeParseCtx(const std::string &src)
    {
        ec->beginScope();
        {
            size_t id = sm->addSourceContent("test", src);
            BasicTokenizer tok(ec, sm);
            tok.tokenizeBuffer(0, id);
            parsingCtx = std::make_shared<BasicParsingContext>(ec, sm, tok.getTokens());
        }
        ec->endScope(ErrorAction::Commit);

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
        ec->endScope(ErrorAction::Commit);
        return dynamic_cast<Module *>(node);
    }

    bool runSemanticPipeline(AstNode *node)
    {
        ec->beginScope();
        {
            SymbolDefinitionVisitor def;
            def.setSemanticContext(semanticCtx);

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
            resolver.setSemanticContext(semanticCtx);

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
            checker.setSemanticContext(semanticCtx);
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

    bool runLowering(AstNode *node)
    {
        bool result = false;
        ec->beginScope();
        {
            AstLowererVisitor astLowerer(loweringCtx);
            TypeLowerer typeLowerer;

            loweringCtx->setOwnerVisitor(&astLowerer);

            result = typeLowerer.lower(loweringCtx->getSemanticContext()->getTypeTable().get(), loweringCtx.get());
            if (!result)
            {
                loweringCtx->getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                                             "Failed to lower types",
                                                             "AstLowererTests");
                ec->endScope(ErrorAction::Commit);
                return false;
            }

            result = node->accept(&astLowerer);
        }
        ec->endScope(ErrorAction::Commit);
        return result;
    }
};

TEST_F(AstLowererTests, EmptyModuleLowersToFunction)
{
    Module *mod = parseModule("void foo() {}");
    ASSERT_NE(mod, nullptr);
    ASSERT_TRUE(runSemanticPipeline(mod));
    ASSERT_TRUE(runLowering(mod));

    Symbol *sym = nullptr;
    ASSERT_TRUE(semanticCtx->resolveSymbolInScope("foo", &sym, true));
    ASSERT_NE(sym, nullptr);

    size_t mirId = loweringCtx->getMirIdOfSymbol(sym);
    ASSERT_NE(mirId, MIRID_INVALID);
}

TEST_F(AstLowererTests, VariableDeclarationLowersToAlloc)
{
    Module *mod = parseModule("void foo() { create i64 %x; }");
    ASSERT_NE(mod, nullptr);
    ASSERT_TRUE(runSemanticPipeline(mod));
    ASSERT_TRUE(runLowering(mod));
}

TEST_F(AstLowererTests, IfStatementLowersCorrectly)
{
    Module *mod = parseModule("void foo() { create i64 %x; if (%x EQ %x) { nop; } else { nop; } }");
    ASSERT_NE(mod, nullptr);
    ASSERT_TRUE(runSemanticPipeline(mod));
    ASSERT_TRUE(runLowering(mod));
}

TEST_F(AstLowererTests, WhileLoopLowersCorrectly)
{
    Module *mod = parseModule("void foo() { create i64 %x; while (%x EQ %x) { nop; } }");
    ASSERT_NE(mod, nullptr);
    ASSERT_TRUE(runSemanticPipeline(mod));
    ASSERT_TRUE(runLowering(mod));
}

TEST_F(AstLowererTests, ForLoopLowersCorrectly)
{
    Module *mod = parseModule("void foo() { for ({create i64 %i; mov %i, 0;} (%i LT 10) {add %i, 1;}) { nop; } }");
    ASSERT_NE(mod, nullptr);
    ASSERT_TRUE(runSemanticPipeline(mod));
    ASSERT_TRUE(runLowering(mod));
}

TEST_F(AstLowererTests, SwitchStatementLowersCorrectly)
{
    Module *mod = parseModule("void foo() { create i64 %x; switch (%x) { case 1: { nop; } default: { nop; } } }");
    ASSERT_NE(mod, nullptr);
    ASSERT_TRUE(runSemanticPipeline(mod));
    ASSERT_TRUE(runLowering(mod));
}

TEST_F(AstLowererTests, BreakAndContinueLowerCorrectly)
{
    Module *mod = parseModule("void foo() { create i64 %x; while (%x EQ %x) { break; continue; } }");
    ASSERT_NE(mod, nullptr);
    ASSERT_TRUE(runSemanticPipeline(mod));
    ASSERT_TRUE(runLowering(mod));
}

TEST_F(AstLowererTests, ReturnLowersCorrectly)
{
    Module *mod = parseModule("void foo() { create i64 %ret; mov %ret, 1; ret %ret; }");
    ASSERT_NE(mod, nullptr);
    ASSERT_TRUE(runSemanticPipeline(mod));
    ASSERT_TRUE(runLowering(mod));
}

TEST_F(AstLowererTests, ArithmeticInstructionsLowerCorrectly)
{
    Module *mod = parseModule("void foo() { create i64 %x; add %x, 1; sub %x, 1; mul %x, 2; div %x, 2; }");
    ASSERT_NE(mod, nullptr);
    ASSERT_TRUE(runSemanticPipeline(mod));
    ASSERT_TRUE(runLowering(mod));
}

TEST_F(AstLowererTests, MemoryOperationsLowerCorrectly)
{
    Module *mod = parseModule("void foo() { create i64 %x; load %x, i64 (%x); store i64 (%x), %x; }");
    ASSERT_NE(mod, nullptr);
    ASSERT_TRUE(runSemanticPipeline(mod));
    ASSERT_TRUE(runLowering(mod));
}

TEST_F(AstLowererTests, FunctionCallWithArgumentsLowersCorrectly)
{
    Module *mod = parseModule("void callee(i64 %a, i64 %b) {} void caller() { call callee(1, 2); }");
    ASSERT_NE(mod, nullptr);
    ASSERT_TRUE(runSemanticPipeline(mod));
    ASSERT_TRUE(runLowering(mod));
}
