#include "FrontendCompilerTestFixture.h"

// =============================================================================
//  TokenizationPhase — valid inputs
// =============================================================================

TEST_F(FrontendCompilerTestFixture, Tokenization_MinimalModule_Succeeds)
{
    auto unit = createUnit("void F() { nop; }", "tok_minimal");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runTokenization(unit.get()));
    EXPECT_NE(unit->getTokenizer(), nullptr);
}

TEST_F(FrontendCompilerTestFixture, Tokenization_ModuleWithAllTypes_Succeeds)
{
    std::string code = R"(
i64 Foo(i8 %a, i16 %b, i32 %c, i64 %d)
{
    create i8 %x;
    create i16 %y;
    create i32 %z;
    create i64 %w;
    nop;
})";
    auto unit = createUnit(code, "tok_all_types");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runTokenization(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Tokenization_HexImmediates_Succeeds)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    mov %x, 0xFF;
    add %x, 0xDEADBEEF;
    sub %x, 0x0;
    nop;
})";
    auto unit = createUnit(code, "tok_hex");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runTokenization(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Tokenization_Comments_Succeeds)
{
    std::string code = R"(
# This is a comment
void F() # inline comment
{
    # Another comment
    nop;
    # End comment
})";
    auto unit = createUnit(code, "tok_comments");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runTokenization(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Tokenization_MemoryOperands_Succeeds)
{
    std::string code = R"(
void F(i64 %ptr)
{
    create i64 %val;
    mov %val, i64 (%ptr+0);
    mov %val, i32 (%ptr+0x10);
    mov %val, i8 (%ptr+0xFF);
    nop;
})";
    auto unit = createUnit(code, "tok_memory");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runTokenization(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Tokenization_ControlFlowKeywords_Succeeds)
{
    std::string code = R"(
void F(i32 %x)
{
    create i32 %i;
    mov %i, 0;
    while (%i LT %x)
    {
        if (%i EQ 5)
        {
            break;
        }
        add %i, 1;
        continue;
    }
    nop;
})";
    auto unit = createUnit(code, "tok_control");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runTokenization(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Tokenization_Labels_Succeeds)
{
    std::string code = R"(
void F()
{
    label_start:
    {
        nop;
    }
    label_end:
    {
        nop;
    }
})";
    auto unit = createUnit(code, "tok_labels");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runTokenization(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Tokenization_TokenizerIsPopulated)
{
    auto unit = createUnit("void F() { nop; }", "tok_populated");
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runTokenization(unit.get()));
    EXPECT_NE(unit->getTokenizer(), nullptr);
    EXPECT_FALSE(unit->getTokenizer()->getTokens().empty());
}

TEST_F(FrontendCompilerTestFixture, Tokenization_AllOpcodes_Succeeds)
{
    std::string code = R"(
void AllOps(i64 %a, i64 %b)
{
    create i64 %r;
    mov %r, %a;
    add %r, %b;
    sub %r, %a;
    mul %r, %b;
    div %r, %a;
    neg %r;
    xor %r, %a;
    and %r, %b;
    or %r, %a;
    shl %r, 1;
    shr %r, 2;
    cmp %a, %b;
    nop;
    ret %r;
})";
    auto unit = createUnit(code, "tok_all_ops");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runTokenization(unit.get()));
}

// =============================================================================
//  TokenizationPhase — edge cases & invalid inputs
// =============================================================================

TEST_F(FrontendCompilerTestFixture, Tokenization_EmptySource_Fails)
{
    auto unit = createUnit("", "tok_empty");
    ASSERT_NE(unit, nullptr);
    // Tokenizer should fail on empty buffer.
    EXPECT_FALSE(runTokenization(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Tokenization_OnlyComments_Succeeds)
{
    std::string code = R"(
# This file contains only comments.
# Nothing else here.
# Just comments everywhere.
)";
    auto unit = createUnit(code, "tok_only_comments");
    ASSERT_NE(unit, nullptr);
    // The tokenizer should be able to handle a file with only comments.
    EXPECT_TRUE(runTokenization(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Tokenization_OnlyWhitespace_Succeeds)
{
    std::string code = "   \n\n   \n  ";
    auto unit = createUnit(code, "tok_whitespace");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runTokenization(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Tokenization_LargeFile_Succeeds)
{
    std::string content = readProgramFile("full_pipeline_stress.ez");
    ASSERT_FALSE(content.empty());
    auto unit = createUnit(content, "tok_large");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runTokenization(unit.get()));
}

