#include "LoweringPipelineTestFixture.h"

// =============================================================================
//  1. File-based: complex_program.ez
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Complex_FileLoads)
{
    ASSERT_TRUE(runFromFile("complex_program.ez"));
}

TEST_F(LoweringPipelineTestFixture, Complex_ProducesModule)
{
    ASSERT_TRUE(runFromFile("complex_program.ez"));
    ASSERT_NE(getModule(), nullptr);
}

TEST_F(LoweringPipelineTestFixture, Complex_ModuleSymbolLinked)
{
    ASSERT_TRUE(runFromFile("complex_program.ez"));
    auto *mod = getModule();
    auto *symAnnot = mod->getAnnotation<SymbolAnnotation>();
    ASSERT_NE(symAnnot, nullptr);
    EXPECT_NE(getMirId(symAnnot->getSymbol()), MIRID_INVALID);
    EXPECT_EQ(symAnnot->getSymbol()->getType(), SymbolType::Module);
}

TEST_F(LoweringPipelineTestFixture, Complex_ParamsLinked)
{
    ASSERT_TRUE(runFromFile("complex_program.ez"));
    expectSymbolLinked("bufferPtr");
    expectSymbolLinked("length");
    expectSymbolLinked("key");
}

TEST_F(LoweringPipelineTestFixture, Complex_ParamsDistinct)
{
    ASSERT_TRUE(runFromFile("complex_program.ez"));
    expectDistinctMirIds("bufferPtr", "length");
    expectDistinctMirIds("length", "key");
    expectDistinctMirIds("bufferPtr", "key");
}

TEST_F(LoweringPipelineTestFixture, Complex_LocalsLinked)
{
    ASSERT_TRUE(runFromFile("complex_program.ez"));
    expectSymbolLinked("runningSum");
    expectSymbolLinked("counter");
    expectSymbolLinked("currentAddr");
    expectSymbolLinked("tempCalc");
}

TEST_F(LoweringPipelineTestFixture, Complex_LocalsDistinct)
{
    ASSERT_TRUE(runFromFile("complex_program.ez"));
    expectDistinctMirIds("runningSum", "counter");
    expectDistinctMirIds("counter", "currentAddr");
    expectDistinctMirIds("currentAddr", "tempCalc");
    expectDistinctMirIds("runningSum", "tempCalc");
}

TEST_F(LoweringPipelineTestFixture, Complex_LabelLinked)
{
    ASSERT_TRUE(runFromFile("complex_program.ez"));
    Symbol *finLabel = resolveInModuleScope("label_finalize");
    ASSERT_NE(finLabel, nullptr);
    EXPECT_TRUE(isSymbolLinked(finLabel));
}

// =============================================================================
//  2. Inline: checksum-like loop
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Complex_Inline_LoopWithAccumulator)
{
    std::string code = R"(
i64 Sum(i32 %n)
{
    create i32 %i;
    create i64 %sum;
    mov %i, 0;
    mov %sum, 0;

    while (%i LT %n)
    {
        add %sum, %i;
        add %i, 1;
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectSymbolLinked("i");
    expectSymbolLinked("sum");
    expectDistinctMirIds("i", "sum");
}

// =============================================================================
//  3. Inline: module with everything
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Complex_Inline_ModuleWithEverything)
{
    std::string code = R"(
i64 BigFunc(i32 %param1, i64 %param2)
{
    create i32 %local1;
    create i64 %local2;
    create i32 %flag;

    mov %local1, %param1;
    mov %local2, %param2;
    mov %flag, 0;

    if (%local1 GT %flag)
    {
        add %local2, 100;
    }
    else
    {
        sub %local2, 100;
    }

    while (%local1 GT %flag)
    {
        sub %local1, 1;
        add %local2, 1;
    }

    finalize:
    {
        and %local2, 0xFFFF;
        nop;
    }
})";
    ASSERT_TRUE(runFromSource(code));
    expectSymbolLinked("param1");
    expectSymbolLinked("param2");
    expectSymbolLinked("local1");
    expectSymbolLinked("local2");
    expectSymbolLinked("flag");
    expectDistinctMirIds("param1", "local1");
    expectDistinctMirIds("param2", "local2");
}

// =============================================================================
//  4. Inline: multiple control structures in sequence
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Complex_Inline_IfThenWhileThenLabel)
{
    std::string code = R"(
void F(i32 %a, i32 %b)
{
    create i32 %result;
    mov %result, 0;

    if (%a GT %b)
    {
        mov %result, 1;
    }

    while (%a GT %result)
    {
        sub %a, 1;
    }

    done:
    {
        add %result, 42;
        nop;
    }
})";
    ASSERT_TRUE(runFromSource(code));
    expectSymbolLinked("result");
}

TEST_F(LoweringPipelineTestFixture, Complex_Inline_WhileThenIfElse)
{
    std::string code = R"(
void F(i32 %x, i32 %limit)
{
    create i32 %acc;
    mov %acc, 0;

    while (%x LT %limit)
    {
        add %acc, %x;
        add %x, 1;
    }

    if (%acc GT %limit)
    {
        sub %acc, %limit;
    }
    else
    {
        add %acc, %limit;
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

// =============================================================================
//  5. Inline: memory + arithmetic + control flow
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Complex_Inline_MemoryInLoop)
{
    std::string code = R"(
void F(i64 %ptr, i32 %n)
{
    create i32 %i;
    create i64 %val;
    create i64 %sum;

    mov %i, 0;
    mov %sum, 0;

    while (%i LT %n)
    {
        mov %val, i64 (%ptr+0);
        add %sum, %val;
        add %ptr, 8;
        add %i, 1;
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectSymbolLinked("val");
    expectSymbolLinked("sum");
}

TEST_F(LoweringPipelineTestFixture, Complex_Inline_BitwiseInIf)
{
    std::string code = R"(
void F(i32 %flags, i32 %mask)
{
    create i32 %result;
    and %result, %flags;

    if (%result EQ %mask)
    {
        or %flags, 0x8000;
    }
    else
    {
        xor %flags, %mask;
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

// =============================================================================
//  6. Inline: many locals used across control flow
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Complex_Inline_ManyLocalsInControlFlow)
{
    std::string code = R"(
void F(i32 %n)
{
    create i32 %a;
    create i32 %b;
    create i32 %c;
    create i32 %d;
    create i32 %i;

    mov %a, 0;
    mov %b, 0;
    mov %c, 0;
    mov %d, 0;
    mov %i, 0;

    while (%i LT %n)
    {
        add %a, 1;
        add %b, 2;
        add %c, 3;
        add %d, 4;
        add %i, 1;
    }

    if (%a GT %b)
    {
        mov %c, %a;
    }
    else
    {
        mov %c, %b;
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectDistinctMirIds("a", "b");
    expectDistinctMirIds("c", "d");
}

// =============================================================================
//  7. Inline: deeply nested everything
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Complex_Inline_DeeplyNested)
{
    std::string code = R"(
void F(i32 %a, i32 %b, i32 %c)
{
    create i32 %r;
    mov %r, 0;

    if (%a GT %b)
    {
        if (%b GT %c)
        {
            while (%r LT %c)
            {
                add %r, 1;
            }
        }
        else
        {
            while (%r LT %b)
            {
                add %r, 2;
            }
        }
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectSymbolLinked("r");
}

// =============================================================================
//  8. Inline: NOP-only module (edge case)
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Complex_Inline_NopOnly)
{
    std::string code = R"(
void F()
{
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Complex_Inline_ManyNops)
{
    std::string code = R"(
void F()
{
    nop;
    nop;
    nop;
    nop;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

