#include "FrontendCompilerTestFixture.h"

// =============================================================================
//  ParsingPhase — valid modules
// =============================================================================

TEST_F(FrontendCompilerTestFixture, Parsing_MinimalModule_Succeeds)
{
    auto unit = createUnit("void F() { nop; }", "parse_minimal");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughParsing(unit.get()));
    EXPECT_NE(unit->getParsingContext(), nullptr);
    EXPECT_NE(unit->getGlobalScopeAstNodes(), nullptr);
}

TEST_F(FrontendCompilerTestFixture, Parsing_ModuleWithParams_Succeeds)
{
    std::string code = R"(
i64 Compute(i32 %a, i64 %b, i32 %c)
{
    create i64 %result;
    mov %result, %b;
    add %result, 1;
    nop;
})";
    auto unit = createUnit(code, "parse_params");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughParsing(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Parsing_VariableDeclarations_Succeeds)
{
    std::string code = R"(
void F()
{
    create i8 %a;
    create i16 %b;
    create i32 %c;
    create i64 %d;
    nop;
})";
    auto unit = createUnit(code, "parse_vars");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughParsing(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Parsing_Labels_Succeeds)
{
    std::string code = R"(
void F()
{
    label_a:
    {
        nop;
    }
    label_b:
    {
        nop;
    }
})";
    auto unit = createUnit(code, "parse_labels");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughParsing(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Parsing_NestedLabels_Succeeds)
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
    auto unit = createUnit(code, "parse_nested_labels");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughParsing(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Parsing_If_Succeeds)
{
    std::string code = R"(
void F(i32 %x)
{
    if (%x EQ 0)
    {
        nop;
    }
})";
    auto unit = createUnit(code, "parse_if");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughParsing(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Parsing_IfElseIfElse_Succeeds)
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
    auto unit = createUnit(code, "parse_if_else");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughParsing(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Parsing_While_Succeeds)
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
    auto unit = createUnit(code, "parse_while");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughParsing(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Parsing_BreakContinue_Succeeds)
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
        continue;
    }
    nop;
})";
    auto unit = createUnit(code, "parse_break_continue");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughParsing(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Parsing_MemoryOperands_Succeeds)
{
    std::string code = R"(
void F(i64 %ptr)
{
    create i64 %val;
    mov %val, i64 (%ptr+0);
    mov %val, i64 (%ptr+0x10);
    nop;
})";
    auto unit = createUnit(code, "parse_memory");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughParsing(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Parsing_AllConditionOps_Succeeds)
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
    auto unit = createUnit(code, "parse_all_conds");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughParsing(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Parsing_VoidNoParams_Succeeds)
{
    auto unit = createUnit("void Empty() { nop; }", "parse_void_no_params");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughParsing(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Parsing_ReturnTypes_Succeeds)
{
    std::string code = R"(
i8 RetI8() { nop; }
i16 RetI16() { nop; }
i32 RetI32() { nop; }
i64 RetI64() { nop; }
void RetVoid() { nop; }
)";
    auto unit = createUnit(code, "parse_rettypes");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughParsing(unit.get()));
}

// =============================================================================
//  ParsingPhase — edge cases & invalid inputs
// =============================================================================

TEST_F(FrontendCompilerTestFixture, Parsing_UnclosedBrace_Fails)
{
    auto unit = createUnit("void F() {", "parse_unclosed");
    ASSERT_NE(unit, nullptr);
    EXPECT_FALSE(runThroughParsing(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Parsing_MissingSemicolon_Fails)
{
    std::string code = R"(
void F()
{
    nop
})";
    auto unit = createUnit(code, "parse_no_semicolon");
    ASSERT_NE(unit, nullptr);
    EXPECT_FALSE(runThroughParsing(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Parsing_InvalidModuleDecl_Fails)
{
    auto unit = createUnit("F() { nop; }", "parse_no_rettype");
    ASSERT_NE(unit, nullptr);
    EXPECT_FALSE(runThroughParsing(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Parsing_GlobalScopeNodes_NotEmpty)
{
    std::string code = R"(
void A() { nop; }
void B() { nop; }
)";
    auto unit = createUnit(code, "parse_multi_module");
    ASSERT_NE(unit, nullptr);
    ASSERT_TRUE(runThroughParsing(unit.get()));
    auto *nodes = unit->getGlobalScopeAstNodes();
    ASSERT_NE(nodes, nullptr);
    EXPECT_GE(nodes->m_numElems, 2u);
}

TEST_F(FrontendCompilerTestFixture, Parsing_LargeFile_Succeeds)
{
    std::string content = readProgramFile("full_pipeline_stress.ez");
    ASSERT_FALSE(content.empty());
    auto unit = createUnit(content, "parse_large");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughParsing(unit.get()));
}

TEST_F(FrontendCompilerTestFixture, Parsing_ManyInstructions_Succeeds)
{
    std::string code = "void F(i64 %a, i64 %b)\n{\n    create i64 %r;\n";
    for (int i = 0; i < 50; ++i)
    {
        code += "    add %r, " + std::to_string(i) + ";\n";
    }
    code += "    nop;\n}\n";
    auto unit = createUnit(code, "parse_many_instr");
    ASSERT_NE(unit, nullptr);
    EXPECT_TRUE(runThroughParsing(unit.get()));
}
