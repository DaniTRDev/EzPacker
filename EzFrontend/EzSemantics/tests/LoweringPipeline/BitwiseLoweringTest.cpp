#include "LoweringPipelineTestFixture.h"

// =============================================================================
//  1. File-based: bitwise.ez – full pipeline
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Bitwise_FileLoads)
{
    ASSERT_TRUE(runFromFile("bitwise.ez"));
}

TEST_F(LoweringPipelineTestFixture, Bitwise_ProducesModule)
{
    ASSERT_TRUE(runFromFile("bitwise.ez"));
    ASSERT_NE(getModule(), nullptr);
}

TEST_F(LoweringPipelineTestFixture, Bitwise_ModuleSymbolLinked)
{
    ASSERT_TRUE(runFromFile("bitwise.ez"));
    auto *mod = getModule();
    auto *symAnnot = mod->getAnnotation<SymbolAnnotation>();
    ASSERT_NE(symAnnot, nullptr);
    EXPECT_NE(getMirId(symAnnot->getSymbol()), MIRID_INVALID);
}

TEST_F(LoweringPipelineTestFixture, Bitwise_ParamLinked)
{
    ASSERT_TRUE(runFromFile("bitwise.ez"));
    expectSymbolLinked("val");
}

TEST_F(LoweringPipelineTestFixture, Bitwise_LocalsLinked)
{
    ASSERT_TRUE(runFromFile("bitwise.ez"));
    expectSymbolLinked("mask");
    expectSymbolLinked("temp");
}

TEST_F(LoweringPipelineTestFixture, Bitwise_LocalsDistinct)
{
    ASSERT_TRUE(runFromFile("bitwise.ez"));
    expectDistinctMirIds("val", "mask");
    expectDistinctMirIds("val", "temp");
    expectDistinctMirIds("mask", "temp");
}

// =============================================================================
//  2. Inline: individual bitwise ops
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Bitwise_Inline_AndVarImm)
{
    std::string code = R"(
void F(i32 %x)
{
    and %x, 0xFF;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Bitwise_Inline_OrVarImm)
{
    std::string code = R"(
void F(i32 %x)
{
    or %x, 0x80;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Bitwise_Inline_XorVarVar)
{
    std::string code = R"(
void F(i32 %a, i32 %b)
{
    xor %a, %b;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Bitwise_Inline_NotUnary)
{
    std::string code = R"(
void F(i32 %x)
{
    not %x;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Bitwise_Inline_ShlVarImm)
{
    std::string code = R"(
void F(i32 %x)
{
    shl %x, 4;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Bitwise_Inline_ShrVarImm)
{
    std::string code = R"(
void F(i32 %x)
{
    shr %x, 1;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Bitwise_Inline_SarVarImm)
{
    std::string code = R"(
void F(i32 %x)
{
    sar %x, 2;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Bitwise_Inline_XorSelf)
{
    std::string code = R"(
void F(i32 %x)
{
    xor %x, %x;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Bitwise_Inline_AndZero)
{
    std::string code = R"(
void F(i32 %x)
{
    and %x, 0;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Bitwise_Inline_OrZero)
{
    std::string code = R"(
void F(i32 %x)
{
    or %x, 0;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Bitwise_Inline_ChainedBitwise)
{
    std::string code = R"(
void F(i32 %x)
{
    and %x, 0xFF;
    or  %x, 0x100;
    xor %x, 0x55;
    not %x;
    shl %x, 1;
    shr %x, 1;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Bitwise_Inline_ShiftZero)
{
    std::string code = R"(
void F(i32 %x)
{
    shl %x, 0;
    shr %x, 0;
    sar %x, 0;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Bitwise_Inline_LargeHexMask)
{
    std::string code = R"(
void F(i64 %x)
{
    and %x, 0xFFFFFFFF;
    or  %x, 0xDEADBEEF;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Bitwise_Inline_MultipleVarsBitwise)
{
    std::string code = R"(
void F(i32 %a, i32 %b)
{
    create i32 %c;
    mov %c, %a;
    and %c, %b;
    or  %c, %a;
    xor %c, %b;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectSymbolLinked("c");
    expectDistinctMirIds("a", "c");
}

TEST_F(LoweringPipelineTestFixture, Bitwise_Inline_i64Bitwise)
{
    std::string code = R"(
void F(i64 %x)
{
    create i64 %mask;
    mov %mask, 0xFFFF;
    and %x, %mask;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

