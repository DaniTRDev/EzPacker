#include "LoweringPipelineTestFixture.h"

// =============================================================================
//  1. File-based: variable_lowering.ez
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Variable_FileLoads)
{
    ASSERT_TRUE(runFromFile("variable_lowering.ez"));
}

TEST_F(LoweringPipelineTestFixture, Variable_ProducesModule)
{
    ASSERT_TRUE(runFromFile("variable_lowering.ez"));
    ASSERT_NE(getModule(), nullptr);
}

TEST_F(LoweringPipelineTestFixture, Variable_ParamLinked)
{
    ASSERT_TRUE(runFromFile("variable_lowering.ez"));
    expectSymbolLinked("input");
}

TEST_F(LoweringPipelineTestFixture, Variable_AllTypesLinked)
{
    ASSERT_TRUE(runFromFile("variable_lowering.ez"));
    expectSymbolLinked("byte");
    expectSymbolLinked("word");
    expectSymbolLinked("dword");
    expectSymbolLinked("qword");
}

TEST_F(LoweringPipelineTestFixture, Variable_AllTypesDistinct)
{
    ASSERT_TRUE(runFromFile("variable_lowering.ez"));
    expectDistinctMirIds("byte", "word");
    expectDistinctMirIds("word", "dword");
    expectDistinctMirIds("dword", "qword");
    expectDistinctMirIds("byte", "qword");
}

TEST_F(LoweringPipelineTestFixture, Variable_ScopedVarInLabel)
{
    ASSERT_TRUE(runFromFile("variable_lowering.ez"));

    // The scoped var lives inside label_scoped
    Symbol *scopedSym = resolveInLabelScope("label_scoped", "scopedVar");
    ASSERT_NE(scopedSym, nullptr);
    EXPECT_TRUE(isSymbolLinked(scopedSym));
}

// =============================================================================
//  2. Inline: create single
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Variable_Inline_CreateI8)
{
    std::string code = R"(
void F()
{
    create i8 %v;
    mov %v, 1;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectSymbolLinked("v");
}

TEST_F(LoweringPipelineTestFixture, Variable_Inline_CreateI16)
{
    std::string code = R"(
void F()
{
    create i16 %v;
    mov %v, 256;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectSymbolLinked("v");
}

TEST_F(LoweringPipelineTestFixture, Variable_Inline_CreateI32)
{
    std::string code = R"(
void F()
{
    create i32 %v;
    mov %v, 100;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectSymbolLinked("v");
}

TEST_F(LoweringPipelineTestFixture, Variable_Inline_CreateI64)
{
    std::string code = R"(
void F()
{
    create i64 %v;
    mov %v, 0xDEADBEEF;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectSymbolLinked("v");
}

// =============================================================================
//  3. Inline: vreg reuse
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Variable_Inline_ReuseVreg)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    mov %x, 1;
    add %x, 2;
    sub %x, 3;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectSymbolLinked("x");
}

TEST_F(LoweringPipelineTestFixture, Variable_Inline_MultipleVarsDistinct)
{
    std::string code = R"(
void F()
{
    create i32 %a;
    create i32 %b;
    create i32 %c;
    mov %a, 1;
    mov %b, 2;
    mov %c, 3;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectDistinctMirIds("a", "b");
    expectDistinctMirIds("b", "c");
    expectDistinctMirIds("a", "c");
}

// =============================================================================
//  4. Inline: params as variables
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Variable_Inline_ParamUsedDirectly)
{
    std::string code = R"(
void F(i32 %x)
{
    add %x, 10;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectSymbolLinked("x");
}

TEST_F(LoweringPipelineTestFixture, Variable_Inline_ParamCopiedToLocal)
{
    std::string code = R"(
void F(i32 %param)
{
    create i32 %local;
    mov %local, %param;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectDistinctMirIds("param", "local");
}

// =============================================================================
//  5. Inline: scoped variables
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Variable_Inline_VarInLabel)
{
    std::string code = R"(
void F()
{
    myLabel:
    {
        create i32 %scopedX;
        mov %scopedX, 99;
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));

    Symbol *sym = resolveInLabelScope("myLabel", "scopedX");
    ASSERT_NE(sym, nullptr);
    EXPECT_TRUE(isSymbolLinked(sym));
}

TEST_F(LoweringPipelineTestFixture, Variable_Inline_SameNameDifferentScopes)
{
    std::string code = R"(
void F()
{
    lbl1:
    {
        create i32 %x;
        mov %x, 1;
    }
    lbl2:
    {
        create i32 %x;
        mov %x, 2;
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));

    Symbol *sym1 = resolveInLabelScope("lbl1", "x");
    Symbol *sym2 = resolveInLabelScope("lbl2", "x");
    ASSERT_NE(sym1, nullptr);
    ASSERT_NE(sym2, nullptr);
    EXPECT_TRUE(isSymbolLinked(sym1));
    EXPECT_TRUE(isSymbolLinked(sym2));
    EXPECT_NE(getMirId(sym1), getMirId(sym2));
}

// =============================================================================
//  6. Inline: many variables
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Variable_Inline_TenLocals)
{
    std::string code = R"(
void F()
{
    create i32 %v0;
    create i32 %v1;
    create i32 %v2;
    create i32 %v3;
    create i32 %v4;
    create i32 %v5;
    create i32 %v6;
    create i32 %v7;
    create i32 %v8;
    create i32 %v9;
    mov %v0, 0;
    mov %v9, 9;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectDistinctMirIds("v0", "v9");
    expectDistinctMirIds("v0", "v1");
}

TEST_F(LoweringPipelineTestFixture, Variable_Inline_MixedTypes)
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
    ASSERT_TRUE(runFromSource(code));
    expectDistinctMirIds("a", "d");
}

