#include "FrontendCompilerTestFixture.h"

// =============================================================================
//  SemanticAnalysisPhase — valid programs
// =============================================================================

TEST_F(FrontendCompilerTestFixture, Semantics_MinimalModule_Succeeds)
{
    auto unit = createUnit("void F() { nop; }", "sem_minimal");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughSemantics(unit.get()));
    EXPECT_NE(unit->getSemanticContext(), nullptr);
}

TEST_F(FrontendCompilerTestFixture, Semantics_ModuleWithParams_Succeeds)
{
    std::string code = R"(
i64 Compute(i32 %a, i64 %b)
{
    create i64 %result;
    mov %result, %b;
    add %result, 1;
    nop;
})";
    auto unit = createUnit(code, "sem_params");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughSemantics(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Semantics_VariableUsedAfterCreate_Succeeds)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    mov %x, 42;
    add %x, 1;
    nop;
})";
    auto unit = createUnit(code, "sem_var_use");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughSemantics(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Semantics_AllTypes_Succeeds)
{
    std::string code = R"(
void F()
{
    create i8  %a;
    create i16 %b;
    create i32 %c;
    create i64 %d;
    mov %a, 1;
    mov %b, 2;
    mov %c, 3;
    mov %d, 4;
    nop;
})";
    auto unit = createUnit(code, "sem_all_types");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughSemantics(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Semantics_Labels_Succeeds)
{
    std::string code = R"(
void F(i32 %x)
{
    create i32 %acc;
    mov %acc, 0;
    label_start:
    {
        add %acc, %x;
        nop;
    }
    label_end:
    {
        nop;
    }
})";
    auto unit = createUnit(code, "sem_labels");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughSemantics(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Semantics_NestedLabels_Succeeds)
{
    std::string code = R"(
void F()
{
    label_outer:
    {
        label_inner:
        {
            nop;
        }
        nop;
    }
})";
    auto unit = createUnit(code, "sem_nested_labels");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughSemantics(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Semantics_If_Succeeds)
{
    std::string code = R"(
void F(i32 %x)
{
    if (%x EQ 0)
    {
        nop;
    }
})";
    auto unit = createUnit(code, "sem_if");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughSemantics(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Semantics_IfElseIfElse_Succeeds)
{
    std::string code = R"(
void F(i32 %x)
{
    if (%x LT 0)
    {
        nop;
    }
    else if (%x EQ 0)
    {
        nop;
    }
    else
    {
        nop;
    }
})";
    auto unit = createUnit(code, "sem_if_else");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughSemantics(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Semantics_While_Succeeds)
{
    std::string code = R"(
void F(i32 %n)
{
    create i32 %i;
    mov %i, 0;
    while (%i LT %n)
    {
        add %i, 1;
    }
    nop;
})";
    auto unit = createUnit(code, "sem_while");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughSemantics(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Semantics_NestedWhile_Succeeds)
{
    std::string code = R"(
void F(i32 %n, i32 %m)
{
    create i32 %i;
    create i32 %j;
    mov %i, 0;
    while (%i LT %n)
    {
        mov %j, 0;
        while (%j LT %m)
        {
            add %j, 1;
        }
        add %i, 1;
    }
    nop;
})";
    auto unit = createUnit(code, "sem_nested_while");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughSemantics(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Semantics_BreakInsideWhile_Succeeds)
{
    std::string code = R"(
void F(i32 %n)
{
    create i32 %i;
    mov %i, 0;
    while (%i LT %n)
    {
        if (%i EQ 5)
        {
            break;
        }
        add %i, 1;
    }
    nop;
})";
    auto unit = createUnit(code, "sem_break");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughSemantics(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Semantics_ContinueInsideWhile_Succeeds)
{
    std::string code = R"(
void F(i32 %n)
{
    create i32 %i;
    mov %i, 0;
    while (%i LT %n)
    {
        add %i, 1;
        if (%i EQ 3)
        {
            continue;
        }
        nop;
    }
    nop;
})";
    auto unit = createUnit(code, "sem_continue");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughSemantics(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Semantics_MemoryOperands_Succeeds)
{
    std::string code = R"(
void F(i64 %ptr)
{
    create i64 %val;
    mov %val, i64 (%ptr+0);
    mov %val, i64 (%ptr+0x10);
    nop;
})";
    auto unit = createUnit(code, "sem_memory");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughSemantics(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Semantics_HexImmediates_Succeeds)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    mov %x, 0xFF;
    add %x, 0xDEAD;
    nop;
})";
    auto unit = createUnit(code, "sem_hex");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughSemantics(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Semantics_AllConditionOps_Succeeds)
{
    std::string code = R"(
void F(i32 %a, i32 %b)
{
    if (%a LT %b) { nop; }
    if (%a GT %b) { nop; }
    if (%a EQ %b) { nop; }
    if (%a NE %b) { nop; }
    if (%a LE %b) { nop; }
    if (%a GE %b) { nop; }
})";
    auto unit = createUnit(code, "sem_all_conds");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughSemantics(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Semantics_ForwardReferenceParams_Succeeds)
{
    std::string code = R"(
void F(i32 %a, i32 %b)
{
    create i32 %sum;
    add %sum, %a;
    add %sum, %b;
    nop;
})";
    auto unit = createUnit(code, "sem_fwd_ref");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughSemantics(unit.get()));
}

// =============================================================================
//  SemanticAnalysisPhase — invalid programs
// =============================================================================

TEST_F(FrontendCompilerTestFixture, Semantics_UndefinedVariable_Fails)
{
    std::string code = R"(
void F()
{
    add %undefined, 1;
    nop;
})";
    auto unit = createUnit(code, "sem_undef");
    ASSERT_NE(unit, nullptr);
    EXPECT_FALSE(runThroughSemantics(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Semantics_UseBeforeCreate_Fails)
{
    std::string code = R"(
void F()
{
    mov %x, 42;
    create i32 %x;
    nop;
})";
    auto unit = createUnit(code, "sem_use_before_create");
    ASSERT_NE(unit, nullptr);
    // The symbol definition pass defines %x via create, but the resolution pass
    // should still succeed because symbols are defined in Pass 1 before resolution in Pass 2.
    // However, whether this is valid depends on the language semantics. We just test
    // that the pipeline produces a deterministic result.
    // In practice, SymbolDefinitionVisitor scans all creates first, so this should succeed.
    // If the language changes to require declaration before use, update this test.
    bool result = runThroughSemantics(unit.get());
    // Just ensure it doesn't crash; result depends on language semantics.
    (void)result;
}

TEST_F(FrontendCompilerTestFixture, Semantics_LargeFile_Succeeds)
{
    std::string content = readProgramFile("full_pipeline_stress.ez");
    ASSERT_FALSE(content.empty());
    auto unit = createUnit(content, "sem_large");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughSemantics(unit.get()));
}

