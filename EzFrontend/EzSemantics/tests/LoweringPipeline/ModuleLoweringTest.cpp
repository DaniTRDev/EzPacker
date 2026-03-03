#include "LoweringPipelineTestFixture.h"

// =============================================================================
//  1. File-based: module_void_no_params.ez
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Module_VoidNoParams_FileLoads)
{
    ASSERT_TRUE(runFromFile("module_void_no_params.ez"));
}

TEST_F(LoweringPipelineTestFixture, Module_VoidNoParams_ProducesModule)
{
    ASSERT_TRUE(runFromFile("module_void_no_params.ez"));
    auto *mod = getModule();
    ASSERT_NE(mod, nullptr);
}

TEST_F(LoweringPipelineTestFixture, Module_VoidNoParams_SymbolLinked)
{
    ASSERT_TRUE(runFromFile("module_void_no_params.ez"));
    auto *mod = getModule();
    auto *symAnnot = mod->getAnnotation<SymbolAnnotation>();
    ASSERT_NE(symAnnot, nullptr);
    EXPECT_NE(getMirId(symAnnot->getSymbol()), MIRID_INVALID);
    EXPECT_EQ(symAnnot->getSymbol()->getType(), SymbolType::Module);
}

// =============================================================================
//  2. File-based: module_with_params.ez
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Module_WithParams_FileLoads)
{
    ASSERT_TRUE(runFromFile("module_with_params.ez"));
}

TEST_F(LoweringPipelineTestFixture, Module_WithParams_ProducesModule)
{
    ASSERT_TRUE(runFromFile("module_with_params.ez"));
    ASSERT_NE(getModule(), nullptr);
}

TEST_F(LoweringPipelineTestFixture, Module_WithParams_ParamsLinked)
{
    ASSERT_TRUE(runFromFile("module_with_params.ez"));
    expectSymbolLinked("paramA");
    expectSymbolLinked("paramB");
    expectSymbolLinked("paramC");
}

TEST_F(LoweringPipelineTestFixture, Module_WithParams_AllDistinct)
{
    ASSERT_TRUE(runFromFile("module_with_params.ez"));
    expectDistinctMirIds("paramA", "paramB");
    expectDistinctMirIds("paramB", "paramC");
    expectDistinctMirIds("paramA", "paramC");
}

TEST_F(LoweringPipelineTestFixture, Module_WithParams_LocalLinked)
{
    ASSERT_TRUE(runFromFile("module_with_params.ez"));
    expectSymbolLinked("local");
    expectDistinctMirIds("local", "paramA");
}

// =============================================================================
//  3. Inline: void modules
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Module_Inline_VoidEmpty)
{
    std::string code = R"(
void F()
{
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    ASSERT_NE(getModule(), nullptr);
}

TEST_F(LoweringPipelineTestFixture, Module_Inline_VoidOnlyNops)
{
    std::string code = R"(
void F()
{
    nop;
    nop;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Module_Inline_VoidWithLocals)
{
    std::string code = R"(
void F()
{
    create i32 %a;
    create i32 %b;
    mov %a, 1;
    mov %b, 2;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectDistinctMirIds("a", "b");
}

// =============================================================================
//  4. Inline: return-type modules
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Module_Inline_i32Return)
{
    std::string code = R"(
i32 F()
{
    create i32 %x;
    mov %x, 42;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectSymbolLinked("x");
}

TEST_F(LoweringPipelineTestFixture, Module_Inline_i64Return)
{
    std::string code = R"(
i64 F(i64 %input)
{
    create i64 %out;
    mov %out, %input;
    add %out, 1;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectDistinctMirIds("input", "out");
}

TEST_F(LoweringPipelineTestFixture, Module_Inline_i8Return)
{
    std::string code = R"(
i8 F(i8 %byte)
{
    add %byte, 1;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectSymbolLinked("byte");
}

// =============================================================================
//  5. Inline: modules with various param counts
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Module_Inline_OneParam)
{
    std::string code = R"(
void F(i32 %x)
{
    add %x, 1;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectSymbolLinked("x");
}

TEST_F(LoweringPipelineTestFixture, Module_Inline_TwoParams)
{
    std::string code = R"(
void F(i32 %a, i64 %b)
{
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectDistinctMirIds("a", "b");
}

TEST_F(LoweringPipelineTestFixture, Module_Inline_FourParams)
{
    std::string code = R"(
void F(i32 %a, i32 %b, i32 %c, i32 %d)
{
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectDistinctMirIds("a", "b");
    expectDistinctMirIds("c", "d");
    expectDistinctMirIds("a", "d");
}

// =============================================================================
//  6. Inline: modules with body complexity
// =============================================================================

TEST_F(LoweringPipelineTestFixture, Module_Inline_WithLabel)
{
    std::string code = R"(
void F()
{
    myLabel:
    {
        nop;
    }
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Module_Inline_WithMultipleLabels)
{
    std::string code = R"(
void F()
{
    lbl1: { nop; }
    lbl2: { nop; }
    lbl3: { nop; }
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Module_Inline_WithIfAndWhile)
{
    std::string code = R"(
void F(i32 %a, i32 %b)
{
    if (%a GT %b)
    {
        nop;
    }
    while (%a LT %b)
    {
        add %a, 1;
    }
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
}

TEST_F(LoweringPipelineTestFixture, Module_Inline_ModuleSymbolIsModuleType)
{
    std::string code = R"(
void MyFunc()
{
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    auto *mod = getModule();
    ASSERT_NE(mod, nullptr);
    auto *symAnnot = mod->getAnnotation<SymbolAnnotation>();
    ASSERT_NE(symAnnot, nullptr);
    EXPECT_EQ(symAnnot->getSymbol()->getType(), SymbolType::Module);
}

TEST_F(LoweringPipelineTestFixture, Module_Inline_ParamsAndLocalsCoexist)
{
    std::string code = R"(
i32 Compute(i32 %p1, i32 %p2)
{
    create i32 %l1;
    create i32 %l2;
    mov %l1, %p1;
    mov %l2, %p2;
    add %l1, %l2;
    nop;
})";
    ASSERT_TRUE(runFromSource(code));
    expectSymbolLinked("p1");
    expectSymbolLinked("p2");
    expectSymbolLinked("l1");
    expectSymbolLinked("l2");
    expectDistinctMirIds("p1", "l1");
    expectDistinctMirIds("p2", "l2");
    expectDistinctMirIds("l1", "l2");
}

