#include "FrontendCompilerTestFixture.h"

// =============================================================================
//  CompilerDriver — Construction
// =============================================================================

TEST_F(FrontendCompilerTestFixture, Driver_Construction_DoesNotThrow)
{
    EXPECT_NO_THROW(auto driver = createDriver());
}

// =============================================================================
//  CompilerDriver — addSource()
// =============================================================================

TEST_F(FrontendCompilerTestFixture, Driver_AddSource_SingleSource_ReturnsTrue)
{
    auto driver = createDriver();
    EXPECT_TRUE(driver->addSource("void F() { nop; }", "source_a"));
}

TEST_F(FrontendCompilerTestFixture, Driver_AddSource_MultipleSources_ReturnTrue)
{
    auto driver = createDriver();
    EXPECT_TRUE(driver->addSource("void A() { nop; }", "source_a"));
    EXPECT_TRUE(driver->addSource("void B() { nop; }", "source_b"));
    EXPECT_TRUE(driver->addSource("void C() { nop; }", "source_c"));
}

TEST_F(FrontendCompilerTestFixture, Driver_AddSource_DuplicateName_ReturnsFalse)
{
    auto driver = createDriver();
    EXPECT_TRUE(driver->addSource("void A() { nop; }", "duplicate"));
    EXPECT_FALSE(driver->addSource("void B() { nop; }", "duplicate"));
}

TEST_F(FrontendCompilerTestFixture, Driver_AddSource_EmptyContent_ReturnsTrue)
{
    auto driver = createDriver();
    EXPECT_TRUE(driver->addSource("", "empty_source"));
}

// =============================================================================
//  CompilerDriver — compile() — valid single source
// =============================================================================

TEST_F(FrontendCompilerTestFixture, Driver_Compile_MinimalVoidModule)
{
    auto driver = createDriver();
    ASSERT_TRUE(driver->addSource("void F() { nop; }", "minimal"));
    EXPECT_TRUE(driver->compile());
}

TEST_F(FrontendCompilerTestFixture, Driver_Compile_ModuleWithParams)
{
    auto driver = createDriver();
    std::string code = R"(
i32 Add(i32 %a, i32 %b)
{
    create i32 %result;
    mov %result, %a;
    add %result, %b;
    nop;
})";
    ASSERT_TRUE(driver->addSource(code, "add_module"));
    EXPECT_TRUE(driver->compile());
}

TEST_F(FrontendCompilerTestFixture, Driver_Compile_ModuleWithControlFlow)
{
    auto driver = createDriver();
    std::string code = R"(
i32 LoopAndBranch(i32 %n)
{
    create i32 %i;
    create i32 %sum;
    mov %i, 0;
    mov %sum, 0;

    while (%i LT %n)
    {
        if (%sum GT 100)
        {
            break;
        }
        add %sum, %i;
        add %i, 1;
    }
    call %LoopAndBranch(1);

    nop;
})";
    ASSERT_TRUE(driver->addSource(code, "loop_branch"));
    EXPECT_TRUE(driver->compile());
}

// =============================================================================
//  CompilerDriver — compile() — multiple sources
// =============================================================================

TEST_F(FrontendCompilerTestFixture, Driver_Compile_TwoSources)
{
    auto driver = createDriver();
    ASSERT_TRUE(driver->addSource("void A() { nop; }", "source_a"));
    ASSERT_TRUE(driver->addSource("void B() { nop; }", "source_b"));
    EXPECT_TRUE(driver->compile());
}

TEST_F(FrontendCompilerTestFixture, Driver_Compile_ManySources)
{
    auto driver = createDriver();
    for (int i = 0; i < 10; ++i)
    {
        std::string name = "module_" + std::to_string(i);
        std::string code = "void " + name + "() { nop; }";
        ASSERT_TRUE(driver->addSource(code, name));
    }
    EXPECT_TRUE(driver->compile());
}

// =============================================================================
//  CompilerDriver — compile() — empty source fails at parsing
// =============================================================================

TEST_F(FrontendCompilerTestFixture, Driver_Compile_EmptySource_FailsTokenization)
{
    auto driver = createDriver();
    ASSERT_TRUE(driver->addSource("", "empty"));
    // An empty buffer has no tokens to tokenize; the tokenizer considers this invalid.
    EXPECT_FALSE(driver->compile());
}

// =============================================================================
//  CompilerDriver — compile() — invalid source
// =============================================================================

TEST_F(FrontendCompilerTestFixture, Driver_Compile_GarbageSource_Fails)
{
    auto driver = createDriver();
    ASSERT_TRUE(driver->addSource("this is not valid ez code!!!", "garbage"));
    EXPECT_FALSE(driver->compile());
}

TEST_F(FrontendCompilerTestFixture, Driver_Compile_UnclosedBrace_Fails)
{
    auto driver = createDriver();
    ASSERT_TRUE(driver->addSource("void F() {", "unclosed"));
    EXPECT_FALSE(driver->compile());
}

TEST_F(FrontendCompilerTestFixture, Driver_Compile_UndefinedVariable_Fails)
{
    auto driver = createDriver();
    std::string code = R"(
void F()
{
    add %undefined, 1;
    nop;
})";
    ASSERT_TRUE(driver->addSource(code, "undef_var"));
    EXPECT_FALSE(driver->compile());
}

// =============================================================================
//  CompilerDriver — compile() — from .ez files
// =============================================================================

TEST_F(FrontendCompilerTestFixture, Driver_Compile_StressProgram)
{
    std::string content = readProgramFile("full_pipeline_stress.ez");
    ASSERT_FALSE(content.empty());
    auto driver = createDriver();
    ASSERT_TRUE(driver->addSource(content, "full_pipeline_stress.ez"));
    EXPECT_TRUE(driver->compile());
}

TEST_F(FrontendCompilerTestFixture, Driver_Compile_NestedControlFlow)
{
    std::string content = readProgramFile("nested_control_flow.ez");
    ASSERT_FALSE(content.empty());
    auto driver = createDriver();
    ASSERT_TRUE(driver->addSource(content, "nested_control_flow.ez"));
    EXPECT_TRUE(driver->compile());
}

TEST_F(FrontendCompilerTestFixture, Driver_Compile_HeavyArithmetic)
{
    std::string content = readProgramFile("heavy_arithmetic.ez");
    ASSERT_FALSE(content.empty());
    auto driver = createDriver();
    ASSERT_TRUE(driver->addSource(content, "heavy_arithmetic.ez"));
    EXPECT_TRUE(driver->compile());
}

TEST_F(FrontendCompilerTestFixture, Driver_Compile_MultiModuleStress)
{
    std::string content = readProgramFile("multi_module_stress.ez");
    ASSERT_FALSE(content.empty());
    auto driver = createDriver();
    ASSERT_TRUE(driver->addSource(content, "multi_module_stress.ez"));
    EXPECT_TRUE(driver->compile());
}

TEST_F(FrontendCompilerTestFixture, Driver_Compile_MemoryIntensive)
{
    std::string content = readProgramFile("memory_intensive.ez");
    ASSERT_FALSE(content.empty());
    auto driver = createDriver();
    ASSERT_TRUE(driver->addSource(content, "memory_intensive.ez"));
    EXPECT_TRUE(driver->compile());
}

TEST_F(FrontendCompilerTestFixture, Driver_Compile_MultipleFilesAtOnce)
{
    std::string stress = readProgramFile("full_pipeline_stress.ez");
    std::string nested = readProgramFile("nested_control_flow.ez");
    std::string arith = readProgramFile("heavy_arithmetic.ez");
    ASSERT_FALSE(stress.empty());
    ASSERT_FALSE(nested.empty());
    ASSERT_FALSE(arith.empty());

    auto driver = createDriver();
    ASSERT_TRUE(driver->addSource(stress, "stress.ez"));
    ASSERT_TRUE(driver->addSource(nested, "nested.ez"));
    ASSERT_TRUE(driver->addSource(arith, "arith.ez"));
    EXPECT_TRUE(driver->compile());
}

