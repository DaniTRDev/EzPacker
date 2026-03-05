#include "FrontendCompilerTestFixture.h"

// =============================================================================
//  FrontendCompilationUnit — Construction & Initialisation
// =============================================================================

TEST_F(FrontendCompilerTestFixture, Unit_DefaultConstruction_HasZeroSourceId)
{
    FrontendCompilationUnit unit(m_errorCollector, m_sourceManager);
    EXPECT_EQ(unit.getTargetSourceId(), 0u);
}

TEST_F(FrontendCompilerTestFixture, Unit_DefaultConstruction_GlobalScopeAstNodesIsNull)
{
    FrontendCompilationUnit unit(m_errorCollector, m_sourceManager);
    EXPECT_EQ(unit.getGlobalScopeAstNodes(), nullptr);
}

TEST_F(FrontendCompilerTestFixture, Unit_DefaultConstruction_TokenizerIsNull)
{
    FrontendCompilationUnit unit(m_errorCollector, m_sourceManager);
    EXPECT_EQ(unit.getTokenizer(), nullptr);
}

TEST_F(FrontendCompilerTestFixture, Unit_DefaultConstruction_ParsingContextIsNull)
{
    FrontendCompilationUnit unit(m_errorCollector, m_sourceManager);
    EXPECT_EQ(unit.getParsingContext(), nullptr);
}

TEST_F(FrontendCompilerTestFixture, Unit_DefaultConstruction_SemanticContextIsNull)
{
    FrontendCompilationUnit unit(m_errorCollector, m_sourceManager);
    EXPECT_EQ(unit.getSemanticContext(), nullptr);
}

TEST_F(FrontendCompilerTestFixture, Unit_DefaultConstruction_MirEmitterIsNull)
{
    FrontendCompilationUnit unit(m_errorCollector, m_sourceManager);
    EXPECT_EQ(unit.getMirEmitter(), nullptr);
}

TEST_F(FrontendCompilerTestFixture, Unit_DefaultConstruction_MirEmitterContextIsNull)
{
    FrontendCompilationUnit unit(m_errorCollector, m_sourceManager);
    EXPECT_EQ(unit.getMirEmitterContext(), nullptr);
}

TEST_F(FrontendCompilerTestFixture, Unit_DefaultConstruction_MirGlobalDataEmitterIsNull)
{
    FrontendCompilationUnit unit(m_errorCollector, m_sourceManager);
    EXPECT_EQ(unit.getMirGlobalDataEmitter(), nullptr);
}

TEST_F(FrontendCompilerTestFixture, Unit_DefaultConstruction_LoweringContextIsNull)
{
    FrontendCompilationUnit unit(m_errorCollector, m_sourceManager);
    EXPECT_EQ(unit.getLoweringContext(), nullptr);
}

// =============================================================================
//  FrontendCompilationUnit — create()
// =============================================================================

TEST_F(FrontendCompilerTestFixture, Unit_Create_ReturnsTrue)
{
    auto unit = createUnit("void F() { nop; }", "test_source");
    ASSERT_NE(unit, nullptr);
}

TEST_F(FrontendCompilerTestFixture, Unit_Create_AssignsNonZeroSourceId)
{
    auto unit = createUnit("void F() { nop; }", "test_source");
    ASSERT_NE(unit, nullptr);
    EXPECT_NE(unit->getTargetSourceId(), 0u);
}

TEST_F(FrontendCompilerTestFixture, Unit_Create_DuplicateNameFails)
{
    m_errorCollector->beginScope();
    auto unitA = createUnit("void A() { nop; }", "duplicate_name");
    ASSERT_NE(unitA, nullptr);

    FrontendCompilationUnit unitB(m_errorCollector, m_sourceManager);
    EXPECT_FALSE(unitB.create("void B() { nop; }", "duplicate_name"));
    m_errorCollector->endScope(ErrorAction::Discard);
}

TEST_F(FrontendCompilerTestFixture, Unit_Create_DifferentNamesSucceed)
{
    auto unitA = createUnit("void A() { nop; }", "source_a");
    auto unitB = createUnit("void B() { nop; }", "source_b");
    ASSERT_NE(unitA, nullptr);
    ASSERT_NE(unitB, nullptr);
    EXPECT_NE(unitA->getTargetSourceId(), unitB->getTargetSourceId());
}

TEST_F(FrontendCompilerTestFixture, Unit_Create_EmptySource)
{
    auto unit = createUnit("", "empty_source");
    ASSERT_NE(unit, nullptr);
    EXPECT_NE(unit->getTargetSourceId(), 0u);
}

// =============================================================================
//  FrontendCompilationUnit — cleanup()
// =============================================================================

TEST_F(FrontendCompilerTestFixture, Unit_Cleanup_ResetsSourceId)
{
    auto unit = createUnit("void F() { nop; }", "cleanup_test");
    ASSERT_NE(unit, nullptr);
    EXPECT_NE(unit->getTargetSourceId(), 0u);
    unit->cleanup();
    EXPECT_EQ(unit->getTargetSourceId(), 0u);
}

TEST_F(FrontendCompilerTestFixture, Unit_Cleanup_ResetsAllComponents)
{
    auto unit = createUnit("void F() { nop; }", "cleanup_components");
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runFullPipeline(unit.get()));

    // All components should be populated after a full pipeline run.
    EXPECT_NE(unit->getTokenizer(), nullptr);
    EXPECT_NE(unit->getParsingContext(), nullptr);
    EXPECT_NE(unit->getSemanticContext(), nullptr);
    EXPECT_NE(unit->getMirEmitter(), nullptr);
    EXPECT_NE(unit->getMirEmitterContext(), nullptr);
    EXPECT_NE(unit->getMirGlobalDataEmitter(), nullptr);
    EXPECT_NE(unit->getLoweringContext(), nullptr);

    unit->cleanup();

    EXPECT_EQ(unit->getTokenizer(), nullptr);
    EXPECT_EQ(unit->getParsingContext(), nullptr);
    EXPECT_EQ(unit->getSemanticContext(), nullptr);
    EXPECT_EQ(unit->getMirEmitter(), nullptr);
    EXPECT_EQ(unit->getMirEmitterContext(), nullptr);
    EXPECT_EQ(unit->getMirGlobalDataEmitter(), nullptr);
    EXPECT_EQ(unit->getLoweringContext(), nullptr);
    EXPECT_EQ(unit->getGlobalScopeAstNodes(), nullptr);
}

// =============================================================================
//  FrontendCompilationUnit — setters
// =============================================================================

TEST_F(FrontendCompilerTestFixture, Unit_SetTokenizer_UpdatesGetter)
{
    FrontendCompilationUnit unit(m_errorCollector, m_sourceManager);
    auto tokenizer = std::make_shared<BasicTokenizer>(m_errorCollector, m_sourceManager);
    unit.setTokenizer(tokenizer);
    EXPECT_EQ(unit.getTokenizer(), tokenizer);
}

TEST_F(FrontendCompilerTestFixture, Unit_SetParsingContext_UpdatesGetter)
{
    FrontendCompilationUnit unit(m_errorCollector, m_sourceManager);
    auto ctx = std::make_shared<BasicParsingContext>(m_errorCollector, m_sourceManager, std::vector<TokenInformation>());
    unit.setParsingContext(ctx);
    EXPECT_EQ(unit.getParsingContext(), ctx);
}

TEST_F(FrontendCompilerTestFixture, Unit_SetSemanticContext_UpdatesGetter)
{
    FrontendCompilationUnit unit(m_errorCollector, m_sourceManager);
    auto ctx = std::make_shared<BasicSemanticContext>(m_errorCollector, m_sourceManager);
    unit.setSemanticContext(ctx);
    EXPECT_EQ(unit.getSemanticContext(), ctx);
}

TEST_F(FrontendCompilerTestFixture, Unit_SetMirEmitterContext_UpdatesGetter)
{
    FrontendCompilationUnit unit(m_errorCollector, m_sourceManager);
    auto ctx = std::make_shared<MirEmitterContext>(m_errorCollector, m_sourceManager);
    unit.setMirEmitterContext(ctx);
    EXPECT_EQ(unit.getMirEmitterContext(), ctx);
}

TEST_F(FrontendCompilerTestFixture, Unit_SetMirEmitter_UpdatesGetter)
{
    FrontendCompilationUnit unit(m_errorCollector, m_sourceManager);
    auto ctx = std::make_shared<MirEmitterContext>(m_errorCollector, m_sourceManager);
    auto emitter = std::make_shared<MirEmitter>(ctx.get());
    unit.setMirEmitter(emitter);
    EXPECT_EQ(unit.getMirEmitter(), emitter);
}

TEST_F(FrontendCompilerTestFixture, Unit_SetMirGlobalDataEmitter_UpdatesGetter)
{
    FrontendCompilationUnit unit(m_errorCollector, m_sourceManager);
    auto ctx = std::make_shared<MirEmitterContext>(m_errorCollector, m_sourceManager);
    auto gde = std::make_shared<MirGlobalDataEmitter>(ctx.get());
    unit.setMirGlobalDataEmitter(gde);
    EXPECT_EQ(unit.getMirGlobalDataEmitter(), gde);
}

