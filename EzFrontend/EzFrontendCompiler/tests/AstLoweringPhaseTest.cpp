#include "FrontendCompilerTestFixture.h"

// =============================================================================
//  1. AstLoweringPhase – getName
// =============================================================================

TEST_F(FrontendCompilerTestFixture, AstLoweringPhase_GetName)
{
    AstLoweringPhase phase;
    EXPECT_STREQ(phase.getName(), "AstLoweringPhase");
}

// =============================================================================
//  2. AstLoweringPhase – valid modules
// =============================================================================

TEST_F(FrontendCompilerTestFixture, AstLowering_SimpleVoidModule)
{
    auto unit = createUnitFromSource("void F() { nop; }");
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runUpToSemantics(unit.get()));

    AstLoweringPhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));

    EXPECT_NE(unit->getMirEmitter(), nullptr);
    EXPECT_NE(unit->getMirEmitterContext(), nullptr);
    EXPECT_NE(unit->getMirGlobalDataEmitter(), nullptr);
    EXPECT_NE(unit->getLoweringContext(), nullptr);
}

TEST_F(FrontendCompilerTestFixture, AstLowering_ModuleWithParams)
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
    ASSERT_TRUE(runUpToSemantics(unit.get()));

    AstLoweringPhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));

    EXPECT_NE(unit->getMirEmitter(), nullptr);
    EXPECT_NE(unit->getLoweringContext(), nullptr);
}

TEST_F(FrontendCompilerTestFixture, AstLowering_ModuleWithVariablesOfAllTypes)
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
    ASSERT_TRUE(runUpToSemantics(unit.get()));

    AstLoweringPhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));
    EXPECT_NE(unit->getMirEmitter(), nullptr);
}

TEST_F(FrontendCompilerTestFixture, AstLowering_ModuleWithArithmetic)
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
    ASSERT_TRUE(runUpToSemantics(unit.get()));

    AstLoweringPhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));
    EXPECT_NE(unit->getMirEmitter(), nullptr);
}

TEST_F(FrontendCompilerTestFixture, AstLowering_ModuleWithControlFlow)
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
    ASSERT_TRUE(runUpToSemantics(unit.get()));

    AstLoweringPhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));
    EXPECT_NE(unit->getMirEmitter(), nullptr);
}

TEST_F(FrontendCompilerTestFixture, AstLowering_ModuleWithLabels)
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
    ASSERT_TRUE(runUpToSemantics(unit.get()));

    AstLoweringPhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));
    EXPECT_NE(unit->getMirEmitter(), nullptr);
}

TEST_F(FrontendCompilerTestFixture, AstLowering_ModuleWithMemoryAccess)
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
    ASSERT_TRUE(runUpToSemantics(unit.get()));

    AstLoweringPhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));
    EXPECT_NE(unit->getMirEmitter(), nullptr);
}

TEST_F(FrontendCompilerTestFixture, AstLowering_OnlyNop)
{
    std::string code = "void F() { nop; }";
    auto unit = createUnitFromSource(code);
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runUpToSemantics(unit.get()));

    AstLoweringPhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));
    EXPECT_NE(unit->getMirEmitter(), nullptr);
}

TEST_F(FrontendCompilerTestFixture, AstLowering_MultipleModulesInOneSource)
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
    ASSERT_TRUE(runUpToSemantics(unit.get()));

    AstLoweringPhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));
    EXPECT_NE(unit->getMirEmitter(), nullptr);
}

// =============================================================================
//  3. AstLoweringPhase – complex programs
// =============================================================================

TEST_F(FrontendCompilerTestFixture, AstLowering_ComplexChecksumProgram)
{
    std::string code = R"(
i64 ComplexChecksum(i64 %bufferPtr, i32 %length, i64 %key)
{
    create i64 %runningSum;
    create i32 %counter;
    create i64 %currentAddr;
    create i64 %tempCalc;

    mov %runningSum, 0;
    mov %counter, 0;
    mov %currentAddr, %bufferPtr;

    while (%counter LT %length)
    {
        mov %tempCalc, i64 (%currentAddr+0);
        xor %tempCalc, 0xFF;
        add %tempCalc, %key;
        add %runningSum, %tempCalc;
        add %currentAddr, 1;
        add %counter, 1;
    }

    if (%runningSum EQ %key)
    {
        mov %runningSum, 0;
    }
    else if (%runningSum GT %key)
    {
        sub %runningSum, %key;
    }
    else
    {
        add %runningSum, %key;
    }

    label_finalize:
    {
        and %runningSum, 0xFFFF;
        nop;
    }
})";
    auto unit = createUnitFromSource(code);
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runUpToSemantics(unit.get()));

    AstLoweringPhase phase;
    EXPECT_TRUE(phase.execute(unit.get()));
    EXPECT_NE(unit->getMirEmitter(), nullptr);
    EXPECT_NE(unit->getLoweringContext(), nullptr);
}

// =============================================================================
//  4. Full single-unit pipeline (all phases)
// =============================================================================

TEST_F(FrontendCompilerTestFixture, FullSingleUnit_SimpleModule)
{
    auto unit = createUnitFromSource("void F() { nop; }");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullSingleUnit(unit.get()));

    // Verify all components are populated
    EXPECT_NE(unit->getTokenizer(), nullptr);
    EXPECT_NE(unit->getParsingContext(), nullptr);
    EXPECT_NE(unit->getSemanticContext(), nullptr);
    EXPECT_NE(unit->getMirEmitter(), nullptr);
    EXPECT_NE(unit->getMirEmitterContext(), nullptr);
    EXPECT_NE(unit->getMirGlobalDataEmitter(), nullptr);
    EXPECT_NE(unit->getLoweringContext(), nullptr);
    EXPECT_NE(unit->getGlobalScopeAstNodes(), nullptr);
}

TEST_F(FrontendCompilerTestFixture, FullSingleUnit_ModuleWithEverything)
{
    std::string code = R"(
i64 FullModule(i32 %x, i64 %y)
{
    create i32 %counter;
    create i64 %result;
    mov %counter, 0;
    mov %result, %y;

    while (%counter LT %x)
    {
        add %result, 1;
        add %counter, 1;
    }

    if (%result GT 100)
    {
        mov %result, 0;
    }
    else
    {
        add %result, %y;
    }

    label_done:
    {
        and %result, 0xFF;
        nop;
    }
})";
    auto unit = createUnitFromSource(code);
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullSingleUnit(unit.get()));
    EXPECT_NE(unit->getMirEmitter(), nullptr);
}

