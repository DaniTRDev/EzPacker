#include "FrontendCompilerTestFixture.h"

// =============================================================================
//  1. SemanticAnalysisPhase – getName
// =============================================================================

TEST_F(FrontendCompilerTestFixture, SemanticAnalysisPhase_GetName)
{
    SemanticAnalysisPhase phase;
    EXPECT_STREQ(phase.getName(), "SemanticAnalysisPhase");
}

// =============================================================================
//  2. Individual sub-phases – getName
// =============================================================================

TEST_F(FrontendCompilerTestFixture, SymbolDefinitionPhase_GetName)
{
    SymbolDefinitionPhase phase;
    EXPECT_STREQ(phase.getName(), "SymbolDefinition");
}

TEST_F(FrontendCompilerTestFixture, SymbolAndTypeResolverPhase_GetName)
{
    SymbolAndTypeResolverPhase phase;
    EXPECT_STREQ(phase.getName(), "SymbolAndTypeResolverPhase");
}

TEST_F(FrontendCompilerTestFixture, TypeCheckPhase_GetName)
{
    TypeCheckPhase phase;
    EXPECT_STREQ(phase.getName(), "TypeCheckPhase");
}

// =============================================================================
//  3. SemanticAnalysisPhase – valid modules
// =============================================================================

TEST_F(FrontendCompilerTestFixture, SemanticAnalysis_SimpleVoidModule)
{
    auto unit = createUnitFromSource("void F() { nop; }");
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runUpToParsing(unit.get()));

    auto semanticContext = std::make_shared<BasicSemanticContext>(getErrorCollector(), getSourceManager(), unit->getGlobalScope());
    unit->setSemanticContext(semanticContext);

    SemanticAnalysisPhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, SemanticAnalysis_ModuleWithParams)
{
    std::string code = R"(
i64 MyModule(i32 %a, i64 %b)
{
    create i32 %local;
    mov %local, %a;
    nop;
})";
    auto unit = createUnitFromSource(code);
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runUpToParsing(unit.get()));

    auto semanticContext = std::make_shared<BasicSemanticContext>(getErrorCollector(), getSourceManager(), unit->getGlobalScope());
    unit->setSemanticContext(semanticContext);

    SemanticAnalysisPhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, SemanticAnalysis_ModuleWithVariablesOfAllTypes)
{
    std::string code = R"(
void F()
{
    create i8  %byte;
    create i16 %word;
    create i32 %dword;
    create i64 %qword;
    mov %byte, 1;
    mov %word, 256;
    mov %dword, 100000;
    mov %qword, 0xDEADBEEF;
    nop;
})";
    auto unit = createUnitFromSource(code);
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runUpToParsing(unit.get()));

    auto semanticContext = std::make_shared<BasicSemanticContext>(getErrorCollector(), getSourceManager(), unit->getGlobalScope());
    unit->setSemanticContext(semanticContext);

    SemanticAnalysisPhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, SemanticAnalysis_ModuleWithControlFlow)
{
    std::string code = R"(
i32 F(i32 %input)
{
    create i32 %result;
    create i32 %counter;
    mov %result, 0;
    mov %counter, 0;

    while (%counter LT %input)
    {
        add %result, %counter;
        add %counter, 1;
    }

    if (%result GT 100)
    {
        mov %result, 100;
    }
    else
    {
        add %result, 1;
    }
    nop;
})";
    auto unit = createUnitFromSource(code);
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runUpToParsing(unit.get()));

    auto semanticContext = std::make_shared<BasicSemanticContext>(getErrorCollector(), getSourceManager(), unit->getGlobalScope());
    unit->setSemanticContext(semanticContext);

    SemanticAnalysisPhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, SemanticAnalysis_ModuleWithArithmetic)
{
    std::string code = R"(
void F()
{
    create i32 %a;
    create i32 %b;
    mov %a, 42;
    mov %b, 10;
    add %a, %b;
    sub %a, 5;
    and %a, 0xFF;
    or  %a, 0x10;
    xor %a, %b;
    shl %a, 2;
    shr %a, 1;
    nop;
})";
    auto unit = createUnitFromSource(code);
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runUpToParsing(unit.get()));

    auto semanticContext = std::make_shared<BasicSemanticContext>(getErrorCollector(), getSourceManager(), unit->getGlobalScope());
    unit->setSemanticContext(semanticContext);

    SemanticAnalysisPhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, SemanticAnalysis_ModuleWithLabels)
{
    std::string code = R"(
void F()
{
    label_start:
    {
        create i32 %localInLabel;
        mov %localInLabel, 42;
        nop;
    }
    label_end:
    {
        nop;
    }
})";
    auto unit = createUnitFromSource(code);
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runUpToParsing(unit.get()));

    auto semanticContext = std::make_shared<BasicSemanticContext>(getErrorCollector(), getSourceManager(), unit->getGlobalScope());
    unit->setSemanticContext(semanticContext);

    SemanticAnalysisPhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, SemanticAnalysis_ModuleWithMemoryAccess)
{
    std::string code = R"(
void F()
{
    create i64 %addr;
    create i64 %val;
    mov %addr, 0;
    mov %val, i64 (%addr+0);
    mov %val, i64 (%addr+0x10);
    nop;
})";
    auto unit = createUnitFromSource(code);
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runUpToParsing(unit.get()));

    auto semanticContext = std::make_shared<BasicSemanticContext>(getErrorCollector(), getSourceManager(), unit->getGlobalScope());
    unit->setSemanticContext(semanticContext);

    SemanticAnalysisPhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, SemanticAnalysis_OnlyNop)
{
    std::string code = R"(
void F()
{
    nop;
})";
    auto unit = createUnitFromSource(code);
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runUpToParsing(unit.get()));

    auto semanticContext = std::make_shared<BasicSemanticContext>(getErrorCollector(), getSourceManager(), unit->getGlobalScope());
    unit->setSemanticContext(semanticContext);

    SemanticAnalysisPhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));
}

// =============================================================================
//  4. Individual sub-phases – SymbolDefinition
// =============================================================================

TEST_F(FrontendCompilerTestFixture, SymbolDefinition_DefinesModuleSymbol)
{
    std::string code = R"(
void MyFunc()
{
    nop;
})";
    auto unit = createUnitFromSource(code);
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runUpToParsing(unit.get()));

    auto semanticContext = std::make_shared<BasicSemanticContext>(getErrorCollector(), getSourceManager(), unit->getGlobalScope());
    unit->setSemanticContext(semanticContext);

    SymbolDefinitionPhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, SymbolDefinition_DefinesVariables)
{
    std::string code = R"(
void F()
{
    create i32 %myVar;
    create i64 %otherVar;
    mov %myVar, 1;
    nop;
})";
    auto unit = createUnitFromSource(code);
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runUpToParsing(unit.get()));

    auto semanticContext = std::make_shared<BasicSemanticContext>(getErrorCollector(), getSourceManager(), unit->getGlobalScope());
    unit->setSemanticContext(semanticContext);

    SymbolDefinitionPhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, SymbolDefinition_DefinesParams)
{
    std::string code = R"(
i32 F(i32 %x, i64 %y)
{
    nop;
})";
    auto unit = createUnitFromSource(code);
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runUpToParsing(unit.get()));

    auto semanticContext = std::make_shared<BasicSemanticContext>(getErrorCollector(), getSourceManager(), unit->getGlobalScope());
    unit->setSemanticContext(semanticContext);

    SymbolDefinitionPhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));
}

// =============================================================================
//  5. SemanticAnalysis – error cases
// =============================================================================

TEST_F(FrontendCompilerTestFixture, SemanticAnalysis_RedefinedVariable)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    create i32 %x;
    nop;
})";
    auto unit = createUnitFromSource(code);
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runUpToParsing(unit.get()));

    auto semanticContext = std::make_shared<BasicSemanticContext>(getErrorCollector(), getSourceManager(), unit->getGlobalScope());
    unit->setSemanticContext(semanticContext);

    SemanticAnalysisPhase phase;
    EXPECT_FALSE(phase.execute(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, SemanticAnalysis_UndefinedVariableUsed)
{
    std::string code = R"(
void F()
{
    mov %undeclared, 1;
    nop;
})";
    auto unit = createUnitFromSource(code);
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runUpToParsing(unit.get()));

    auto semanticContext = std::make_shared<BasicSemanticContext>(getErrorCollector(), getSourceManager(), unit->getGlobalScope());
    unit->setSemanticContext(semanticContext);

    SemanticAnalysisPhase phase;
    EXPECT_FALSE(phase.execute(unit.get()));
}

// =============================================================================
//  6. SemanticAnalysis – multiple modules in one source
// =============================================================================

TEST_F(FrontendCompilerTestFixture, SemanticAnalysis_MultipleModules)
{
    std::string code = R"(
void A()
{
    create i32 %x;
    mov %x, 1;
    nop;
}

void B()
{
    create i32 %y;
    mov %y, 2;
    nop;
})";
    auto unit = createUnitFromSource(code);
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runUpToParsing(unit.get()));

    auto semanticContext = std::make_shared<BasicSemanticContext>(getErrorCollector(), getSourceManager(), unit->getGlobalScope());
    unit->setSemanticContext(semanticContext);

    SemanticAnalysisPhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));
}

