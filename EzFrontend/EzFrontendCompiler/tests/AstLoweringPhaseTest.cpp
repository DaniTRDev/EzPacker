#include "FrontendCompilerTestFixture.h"

// =============================================================================
//  AstLoweringPhase — valid programs
// =============================================================================

TEST_F(FrontendCompilerTestFixture, Lowering_MinimalModule_Succeeds)
{
    auto unit = createUnit("void F() { nop; }", "low_minimal");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullPipeline(unit.get()));
    EXPECT_NE(unit->getMirEmitter(), nullptr);
    EXPECT_NE(unit->getMirEmitterContext(), nullptr);
    EXPECT_NE(unit->getMirGlobalDataEmitter(), nullptr);
    EXPECT_NE(unit->getLoweringContext(), nullptr);
}

TEST_F(FrontendCompilerTestFixture, Lowering_ArithmeticOps_Succeeds)
{
    std::string code = R"(
void F(i32 %a, i32 %b)
{
    create i32 %r;
    mov %r, %a;
    add %r, %b;
    sub %r, 1;
    mul %r, 2;
    div %r, 3;
    neg %r;
    nop;
})";
    auto unit = createUnit(code, "low_arith");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullPipeline(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Lowering_BitwiseOps_Succeeds)
{
    std::string code = R"(
void F(i64 %a, i64 %b)
{
    create i64 %r;
    mov %r, %a;
    xor %r, %b;
    and %r, 0xFF;
    or %r, 0x100;
    shl %r, 2;
    shr %r, 1;
    nop;
})";
    auto unit = createUnit(code, "low_bitwise");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullPipeline(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Lowering_DataMovement_Succeeds)
{
    std::string code = R"(
void F(i64 %src, i64 %dst)
{
    create i64 %val;
    mov %val, 0;
    mov %val, %src;
    mov %val, i64 (%src+0);
    mov %val, i64 (%src+0x10);
    nop;
})";
    auto unit = createUnit(code, "low_data_mov");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullPipeline(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Lowering_Labels_Succeeds)
{
    std::string code = R"(
void F(i32 %x)
{
    create i32 %acc;
    mov %acc, 0;
    label_init:
    {
        mov %acc, %x;
    }
    label_compute:
    {
        add %acc, 10;
        nop;
    }
})";
    auto unit = createUnit(code, "low_labels");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullPipeline(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Lowering_NestedLabels_Succeeds)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    mov %x, 0;
    label_outer:
    {
        add %x, 1;
        label_inner:
        {
            add %x, 2;
            nop;
        }
        sub %x, 1;
    }
    nop;
})";
    auto unit = createUnit(code, "low_nested_labels");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullPipeline(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Lowering_If_Succeeds)
{
    std::string code = R"(
void F(i32 %x)
{
    if (%x EQ 0)
    {
        nop;
    }
})";
    auto unit = createUnit(code, "low_if");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullPipeline(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Lowering_IfElseIfElse_Succeeds)
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
    auto unit = createUnit(code, "low_if_else");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullPipeline(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Lowering_While_Succeeds)
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
    auto unit = createUnit(code, "low_while");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullPipeline(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Lowering_NestedWhile_Succeeds)
{
    std::string code = R"(
void F(i32 %n, i32 %m)
{
    create i32 %i;
    create i32 %j;
    create i32 %sum;
    mov %i, 0;
    mov %sum, 0;
    while (%i LT %n)
    {
        mov %j, 0;
        while (%j LT %m)
        {
            add %sum, 1;
            add %j, 1;
        }
        add %i, 1;
    }
    nop;
})";
    auto unit = createUnit(code, "low_nested_while");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullPipeline(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Lowering_Break_Succeeds)
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
    auto unit = createUnit(code, "low_break");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullPipeline(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Lowering_Continue_Succeeds)
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
    auto unit = createUnit(code, "low_continue");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullPipeline(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Lowering_MemoryOperands_Succeeds)
{
    std::string code = R"(
void F(i64 %ptr)
{
    create i64 %val;
    create i32 %small;
    mov %val, i64 (%ptr+0);
    mov %val, i64 (%ptr+0x10);
    mov %small, i32 (%ptr+0x20);
    nop;
})";
    auto unit = createUnit(code, "low_memory");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullPipeline(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Lowering_HexImmediates_Succeeds)
{
    std::string code = R"(
void F()
{
    create i64 %x;
    mov %x, 0xDEADBEEF;
    add %x, 0xFF;
    and %x, 0xFFFF;
    nop;
})";
    auto unit = createUnit(code, "low_hex");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullPipeline(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Lowering_ZeroImmediate_Succeeds)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    mov %x, 0;
    add %x, 0;
    nop;
})";
    auto unit = createUnit(code, "low_zero");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullPipeline(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Lowering_MultipleVariablesSameType_Succeeds)
{
    std::string code = R"(
void F()
{
    create i64 %a;
    create i64 %b;
    create i64 %c;
    create i64 %d;
    mov %a, 1;
    mov %b, 2;
    mov %c, 3;
    mov %d, 4;
    add %a, %b;
    add %c, %d;
    nop;
})";
    auto unit = createUnit(code, "low_multi_vars");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullPipeline(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Lowering_CombinedProgram_Succeeds)
{
    std::string code = R"(
i64 Combined(i64 %bufPtr, i32 %len, i64 %key)
{
    create i64 %sum;
    create i32 %i;
    create i64 %addr;
    create i64 %temp;

    mov %sum, 0;
    mov %i, 0;
    mov %addr, %bufPtr;

    while (%i LT %len)
    {
        mov %temp, i64 (%addr+0);
        xor %temp, 0xFF;
        add %temp, %key;
        add %sum, %temp;
        add %addr, 1;
        add %i, 1;
    }

    if (%sum EQ %key)
    {
        mov %sum, 0;
    }
    else if (%sum GT %key)
    {
        sub %sum, %key;
    }
    else
    {
        add %sum, %key;
    }

    label_finalize:
    {
        and %sum, 0xFFFF;
        nop;
    }
})";
    auto unit = createUnit(code, "low_combined");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullPipeline(unit.get()));
}

// =============================================================================
//  AstLoweringPhase — file-based tests
// =============================================================================

TEST_F(FrontendCompilerTestFixture, Lowering_StressFile_Succeeds)
{
    std::string content = readProgramFile("full_pipeline_stress.ez");
    ASSERT_FALSE(content.empty());
    auto unit = createUnit(content, "low_stress");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullPipeline(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Lowering_NestedControlFlowFile_Succeeds)
{
    std::string content = readProgramFile("nested_control_flow.ez");
    ASSERT_FALSE(content.empty());
    auto unit = createUnit(content, "low_nested_cf");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullPipeline(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Lowering_HeavyArithmeticFile_Succeeds)
{
    std::string content = readProgramFile("heavy_arithmetic.ez");
    ASSERT_FALSE(content.empty());
    auto unit = createUnit(content, "low_heavy_arith");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullPipeline(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Lowering_MemoryIntensiveFile_Succeeds)
{
    std::string content = readProgramFile("memory_intensive.ez");
    ASSERT_FALSE(content.empty());
    auto unit = createUnit(content, "low_mem_intensive");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullPipeline(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Lowering_MultiModuleStressFile_Succeeds)
{
    std::string content = readProgramFile("multi_module_stress.ez");
    ASSERT_FALSE(content.empty());
    auto unit = createUnit(content, "low_multi_mod");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runFullPipeline(unit.get()));
}

