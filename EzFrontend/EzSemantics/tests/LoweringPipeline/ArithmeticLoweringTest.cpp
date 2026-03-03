#include "LoweringPipelineTestFixture.h"

// =============================================================================
//  1. File-based: arithmetic.ez – full pipeline
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Arithmetic_FileLoads)
{
    ASSERT_TRUE(runFromFile("arithmetic.ez"));
}

TEST_F(LoweringPipelineTestFixture, Arithmetic_ProducesModule)
{
    ASSERT_TRUE(runFromFile("arithmetic.ez"));
    ASSERT_NE(getModule(), nullptr);
    EXPECT_EQ(getAstNode()->getType(), AstNodeType::Module);
}

TEST_F(LoweringPipelineTestFixture, Arithmetic_ModuleSymbolLinked)
{
    ASSERT_TRUE(runFromFile("arithmetic.ez"));
    auto *mod = getModule();
    ASSERT_NE(mod, nullptr);

    auto *symAnnot = mod->getAnnotation<SymbolAnnotation>();
    ASSERT_NE(symAnnot, nullptr);
    EXPECT_NE(getMirId(symAnnot->getSymbol()), MIRID_INVALID);
}

TEST_F(LoweringPipelineTestFixture, Arithmetic_ParamsLinked)
{
    ASSERT_TRUE(runFromFile("arithmetic.ez"));
    expectSymbolLinked("a");
    expectSymbolLinked("b");
}

TEST_F(LoweringPipelineTestFixture, Arithmetic_ParamsDistinctIds)
{
    ASSERT_TRUE(runFromFile("arithmetic.ez"));
    expectDistinctMirIds("a", "b");
}

TEST_F(LoweringPipelineTestFixture, Arithmetic_LocalResultLinked)
{
    ASSERT_TRUE(runFromFile("arithmetic.ez"));
    expectSymbolLinked("result");
}

TEST_F(LoweringPipelineTestFixture, Arithmetic_AllSymbolsDistinct)
{
    ASSERT_TRUE(runFromFile("arithmetic.ez"));
    expectDistinctMirIds("a", "result");
    expectDistinctMirIds("b", "result");
}

// =============================================================================
//  2. Inline: individual arithmetic ops – edge cases
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Arithmetic_Inline_AddVarImm)
{
    std::string code = R"(
void F(i32 %x)
{
    create i32 %v;
    add %v, 10;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectSymbolLinked("v");
}

TEST_F(LoweringPipelineTestFixture, Arithmetic_Inline_SubVarVar)
{
    std::string code = R"(
void F(i32 %a, i32 %b)
{
    sub %a, %b;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Arithmetic_Inline_MulVarImm)
{
    std::string code = R"(
void F(i32 %x)
{
    mul %x, 3;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Arithmetic_Inline_DivVarImm)
{
    std::string code = R"(
void F(i32 %x)
{
    div %x, 2;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Arithmetic_Inline_NegUnary)
{
    std::string code = R"(
void F(i32 %x)
{
    neg %x;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Arithmetic_Inline_AddZero)
{
    std::string code = R"(
void F(i32 %x)
{
    add %x, 0;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Arithmetic_Inline_SubSelf)
{
    std::string code = R"(
void F(i32 %x)
{
    sub %x, %x;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Arithmetic_Inline_ChainedOps)
{
    std::string code = R"(
void F(i32 %x)
{
    add %x, 1;
    add %x, 2;
    add %x, 3;
    sub %x, 6;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Arithmetic_Inline_MulByOne)
{
    std::string code = R"(
void F(i32 %x)
{
    mul %x, 1;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Arithmetic_Inline_DivByOne)
{
    std::string code = R"(
void F(i32 %x)
{
    div %x, 1;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Arithmetic_Inline_HexImmediate)
{
    std::string code = R"(
void F(i32 %x)
{
    add %x, 0xFF;
    sub %x, 0xAB;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Arithmetic_Inline_MultipleVarsArithmetic)
{
    std::string code = R"(
void F(i32 %a, i32 %b, i32 %c)
{
    create i32 %sum;
    mov %sum, %a;
    add %sum, %b;
    add %sum, %c;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectSymbolLinked("sum");
    expectDistinctMirIds("a", "sum");
    expectDistinctMirIds("b", "sum");
    expectDistinctMirIds("c", "sum");
}

TEST_F(LoweringPipelineTestFixture, Arithmetic_Inline_i64Arithmetic)
{
    std::string code = R"(
void F(i64 %big)
{
    create i64 %result;
    mov %result, %big;
    add %result, 0xDEADBEEF;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectSymbolLinked("result");
}

TEST_F(LoweringPipelineTestFixture, Arithmetic_Inline_InvalidMnemonicFails)
{
    std::string code = R"(
void F(i32 %x)
{
    foobar %x, 1;
    nop;
})";
    EXPECT_FALSE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Arithmetic_Inline_AddThenNeg)
{
    std::string code = R"(
void F(i32 %x)
{
    add %x, 100;
    neg %x;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

