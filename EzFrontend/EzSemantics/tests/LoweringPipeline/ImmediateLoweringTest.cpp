#include "LoweringPipelineTestFixture.h"

// =============================================================================
//  1. File-based: immediate_lowering.ez
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Immediate_FileLoads)
{
    ASSERT_TRUE(runFromFile("immediate_lowering.ez"));
}

TEST_F(LoweringPipelineTestFixture, Immediate_ProducesModule)
{
    ASSERT_TRUE(runFromFile("immediate_lowering.ez"));
    ASSERT_NE(getModule(), nullptr);
}

TEST_F(LoweringPipelineTestFixture, Immediate_LocalsLinked)
{
    ASSERT_TRUE(runFromFile("immediate_lowering.ez"));
    expectSymbolLinked("a");
    expectSymbolLinked("b");
    expectSymbolLinked("c");
}

TEST_F(LoweringPipelineTestFixture, Immediate_AllDistinct)
{
    ASSERT_TRUE(runFromFile("immediate_lowering.ez"));
    expectDistinctMirIds("a", "b");
    expectDistinctMirIds("b", "c");
}

// =============================================================================
//  2. Inline: small integer immediates
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Immediate_Inline_Zero)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    mov %x, 0;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Immediate_Inline_One)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    mov %x, 1;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Immediate_Inline_Small)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    mov %x, 42;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Immediate_Inline_Max8)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    mov %x, 255;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Immediate_Inline_Max16)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    mov %x, 65535;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

// =============================================================================
//  3. Inline: hex immediates
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Immediate_Inline_Hex_0xFF)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    mov %x, 0xFF;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Immediate_Inline_Hex_0xDEAD)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    mov %x, 0xDEAD;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Immediate_Inline_Hex_0xDEADBEEF)
{
    std::string code = R"(
void F()
{
    create i64 %x;
    mov %x, 0xDEADBEEF;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Immediate_Inline_Hex_0xCAFE)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    mov %x, 0xCAFE;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

// =============================================================================
//  4. Inline: immediates in different instructions
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Immediate_Inline_InAdd)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    add %x, 100;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Immediate_Inline_InSub)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    sub %x, 50;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Immediate_Inline_InAnd)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    and %x, 0xFF;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Immediate_Inline_InOr)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    or %x, 0x8000;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Immediate_Inline_InXor)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    xor %x, 0x55;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Immediate_Inline_InShl)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    shl %x, 8;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Immediate_Inline_InShr)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    shr %x, 4;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

// =============================================================================
//  5. Inline: multiple immediates in sequence
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Immediate_Inline_MultipleInSequence)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    mov %x, 0;
    add %x, 1;
    add %x, 10;
    add %x, 100;
    add %x, 0xFF;
    add %x, 0xABC;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Immediate_Inline_MixedHexDecimal)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    mov %x, 42;
    add %x, 0x10;
    sub %x, 7;
    and %x, 0xFF;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

// =============================================================================
//  6. Inline: edge cases
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Immediate_Inline_LargeHex64)
{
    std::string code = R"(
void F()
{
    create i64 %x;
    mov %x, 0xFFFFFFFF;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Immediate_Inline_ImmediateAsSecondOp)
{
    std::string code = R"(
void F(i32 %a)
{
    create i32 %b;
    mov %b, %a;
    add %b, 0xBEEF;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

