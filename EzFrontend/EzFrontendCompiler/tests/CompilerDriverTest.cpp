#include "FrontendCompilerTestFixture.h"

// =============================================================================
//  1. CompilerDriver – addSource
// =============================================================================

TEST_F(FrontendCompilerTestFixture, CompilerDriver_AddSource_ValidSource)
{
    std::shared_ptr<FrontendCompilationUnit> outUnit;
    EXPECT_TRUE(getDriver()->addSource("void F() { nop; }", "test.ez", &outUnit));
    EXPECT_NE(outUnit, nullptr);
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_AddSource_DuplicateNameWarnsAndReturnsTrue)
{
    EXPECT_TRUE(getDriver()->addSource("void F() { nop; }", "dup.ez"));
    // Adding the same name again should succeed (with warning) but not add a new unit
    EXPECT_TRUE(getDriver()->addSource("void G() { nop; }", "dup.ez"));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_AddSource_OutUnitIsNullOptional)
{
    // Pass nullptr for outUnit — should still work
    EXPECT_TRUE(getDriver()->addSource("void F() { nop; }", "no_out.ez", nullptr));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_AddSource_MultipleSources)
{
    std::shared_ptr<FrontendCompilationUnit> u1, u2, u3;
    EXPECT_TRUE(getDriver()->addSource("void A() { nop; }", "a.ez", &u1));
    EXPECT_TRUE(getDriver()->addSource("void B() { nop; }", "b.ez", &u2));
    EXPECT_TRUE(getDriver()->addSource("void C() { nop; }", "c.ez", &u3));
    EXPECT_NE(u1, nullptr);
    EXPECT_NE(u2, nullptr);
    EXPECT_NE(u3, nullptr);
}

// =============================================================================
//  2. CompilerDriver – addSourceFromFile
// =============================================================================

TEST_F(FrontendCompilerTestFixture, CompilerDriver_AddSourceFromFile_ValidFile)
{
    auto path = "simple_module.ez";
    EXPECT_TRUE(getDriver()->addSourceFromFile(path));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_AddSourceFromFile_NonExistentFile)
{
    EXPECT_FALSE(getDriver()->addSourceFromFile("nonexistent_file_xyz.ez"));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_AddSourceFromFile_DirectoryPath)
{
    auto dirPath = getProgramsDir();
    EXPECT_FALSE(getDriver()->addSourceFromFile(dirPath.string()));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_AddSourceFromFile_DuplicateFileReturnsTrue)
{
    auto path = "simple_module.ez";
    EXPECT_TRUE(getDriver()->addSourceFromFile(path));
    // Adding same file again should skip with warning but return true
    EXPECT_TRUE(getDriver()->addSourceFromFile(path));
}

// =============================================================================
//  3. CompilerDriver – compile single source
// =============================================================================

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_SimpleModule)
{
    EXPECT_TRUE(compileFromSource("void F() { nop; }"));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_ModuleWithParams)
{
    std::string code = R"(
i64 MyModule(i32 %a, i64 %b)
{
    create i32 %local;
    mov %local, %a;
    add %local, 1;
    nop;
})";
    EXPECT_TRUE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_ModuleWithVariables)
{
    std::string code = R"(
void F()
{
    create i8  %byte;
    create i16 %word;
    create i32 %dword;
    create i64 %qword;
    mov %byte, 1;
    mov %word, 256;
    mov %dword, 100000;
    mov %qword, 0xDEADBEEF;
    nop;
})";
    EXPECT_TRUE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_ModuleWithControlFlow)
{
    std::string code = R"(
i32 F(i32 %input)
{
    create i32 %result;
    create i32 %counter;
    mov %result, 0;
    mov %counter, 0;

    while (%counter LT %input)
    {
        add %result, %counter;
        add %counter, 1;
    }

    if (%result GT 100)
    {
        mov %result, 100;
    }
    else
    {
        add %result, 1;
    }
    nop;
})";
    EXPECT_TRUE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_ModuleWithArithmetic)
{
    std::string code = R"(
void F()
{
    create i32 %a;
    create i32 %b;
    mov %a, 42;
    mov %b, 10;
    add %a, %b;
    sub %a, 5;
    and %a, 0xFF;
    or  %a, 0x10;
    xor %a, %b;
    shl %a, 2;
    shr %a, 1;
    nop;
})";
    EXPECT_TRUE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_ModuleWithLabels)
{
    std::string code = R"(
void F()
{
    label_start:
    {
        create i32 %localInLabel;
        mov %localInLabel, 42;
        nop;
    }
    label_end:
    {
        nop;
    }
})";
    EXPECT_TRUE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_ModuleWithMemoryAccess)
{
    std::string code = R"(
void F()
{
    create i64 %addr;
    create i64 %val;
    mov %addr, 0;
    mov %val, i64 (%addr+0);
    mov %val, i64 (%addr+0x10);
    nop;
})";
    EXPECT_TRUE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_ModuleOnlyNop)
{
    EXPECT_TRUE(compileFromSource("void F() { nop; }"));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_MultipleModulesInOneSource)
{
    std::string code = R"(
void A()
{
    create i32 %x;
    mov %x, 1;
    nop;
}

void B()
{
    create i32 %y;
    mov %y, 2;
    nop;
}

i64 C(i32 %param)
{
    create i64 %z;
    mov %z, 0;
    add %z, 1;
    nop;
})";
    EXPECT_TRUE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_ComplexChecksumProgram)
{
    std::string code = R"(
i64 ComplexChecksum(i64 %bufferPtr, i32 %length, i64 %key)
{
    create i64 %runningSum;
    create i32 %counter;
    create i64 %currentAddr;
    create i64 %tempCalc;

    mov %runningSum, 0;
    mov %counter, 0;
    mov %currentAddr, %bufferPtr;

    while (%counter LT %length)
    {
        mov %tempCalc, i64 (%currentAddr+0);
        xor %tempCalc, 0xFF;
        add %tempCalc, %key;
        add %runningSum, %tempCalc;
        add %currentAddr, 1;
        add %counter, 1;
    }

    if (%runningSum EQ %key)
    {
        mov %runningSum, 0;
    }
    else if (%runningSum GT %key)
    {
        sub %runningSum, %key;
    }
    else
    {
        add %runningSum, %key;
    }

    label_finalize:
    {
        and %runningSum, 0xFFFF;
        nop;
    }
})";
    EXPECT_TRUE(compileFromSource(code));
}

// =============================================================================
//  4. CompilerDriver – compile from file
// =============================================================================

TEST_F(FrontendCompilerTestFixture, CompilerDriver_CompileFromFile_SimpleModule)
{
    EXPECT_TRUE(compileFromFile("simple_module.ez"));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_CompileFromFile_ModuleWithParams)
{
    EXPECT_TRUE(compileFromFile("module_with_params.ez"));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_CompileFromFile_ModuleWithVariables)
{
    EXPECT_TRUE(compileFromFile("module_with_variables.ez"));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_CompileFromFile_ModuleWithControlFlow)
{
    EXPECT_TRUE(compileFromFile("module_with_control_flow.ez"));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_CompileFromFile_ModuleWithArithmetic)
{
    EXPECT_TRUE(compileFromFile("module_with_arithmetic.ez"));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_CompileFromFile_ComplexFullPipeline)
{
    EXPECT_TRUE(compileFromFile("complex_full_pipeline.ez"));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_CompileFromFile_MinimalModule)
{
    EXPECT_TRUE(compileFromFile("minimal_module.ez"));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_CompileFromFile_MultipleModules)
{
    EXPECT_TRUE(compileFromFile("multiple_modules.ez"));
}

// =============================================================================
//  5. CompilerDriver – error cases
// =============================================================================

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_SyntaxErrorFails)
{
    std::string code = R"(
void F()
{
    create i32 %a
    mov %a, 1;
    nop;
})";
    EXPECT_FALSE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_MissingClosingBraceFails)
{
    std::string code = R"(
void F()
{
    nop;
)";
    EXPECT_FALSE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_UndefinedVariableFails)
{
    std::string code = R"(
void F()
{
    mov %undeclared, 1;
    nop;
})";
    EXPECT_FALSE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_RedefinedVariableFails)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    create i32 %x;
    nop;
})";
    EXPECT_FALSE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_InvalidReturnTypeFails)
{
    std::string code = R"(
invalidType F()
{
    nop;
})";
    EXPECT_FALSE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_CompileFromFile_SyntaxErrorFile)
{
    EXPECT_FALSE(compileFromFile("syntax_error.ez"));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_CompileFromFile_NonExistentFileFails)
{
    EXPECT_FALSE(compileFromFile("nonexistent.ez"));
}

// =============================================================================
//  6. CompilerDriver – multiple independent sources
// =============================================================================

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_TwoIndependentSources)
{
    std::string codeA = R"(
void A()
{
    create i32 %x;
    mov %x, 1;
    nop;
})";
    std::string codeB = R"(
void B()
{
    create i32 %y;
    mov %y, 2;
    nop;
})";
    ASSERT_TRUE(getDriver()->addSource(codeA, "a.ez"));
    ASSERT_TRUE(getDriver()->addSource(codeB, "b.ez"));
    EXPECT_TRUE(getDriver()->compile());
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_ThreeIndependentSources)
{
    ASSERT_TRUE(getDriver()->addSource("void A() { nop; }", "a.ez"));
    ASSERT_TRUE(getDriver()->addSource("void B() { nop; }", "b.ez"));
    ASSERT_TRUE(getDriver()->addSource("void C() { nop; }", "c.ez"));
    EXPECT_TRUE(getDriver()->compile());
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_MixOfInlineAndFileSources)
{
    ASSERT_TRUE(getDriver()->addSource("void InlineModule() { nop; }", "inline.ez"));
    auto path = "simple_module.ez";
    ASSERT_TRUE(getDriver()->addSourceFromFile(path));
    EXPECT_TRUE(getDriver()->compile());
}

// =============================================================================
//  7. CompilerDriver – edge cases
// =============================================================================

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_ModuleWithManyNops)
{
    std::string code = R"(
void F()
{
    nop;
    nop;
    nop;
    nop;
    nop;
    nop;
    nop;
    nop;
    nop;
    nop;
})";
    EXPECT_TRUE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_NestedIfElse)
{
    std::string code = R"(
void F()
{
    create i32 %a;
    create i32 %b;
    mov %a, 5;
    mov %b, 10;

    if (%a LT %b)
    {
        if (%a GT 0)
        {
            sub %a, 1;
        }
        else
        {
            add %a, 1;
        }
    }
    else
    {
        mov %a, 0;
    }
    nop;
})";
    EXPECT_TRUE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_WhileWithBreak)
{
    std::string code = R"(
void F()
{
    create i32 %i;
    mov %i, 0;
    while (%i LT 100)
    {
        add %i, 1;
        if (%i EQ 50)
        {
            break;
        }
    }
    nop;
})";
    EXPECT_TRUE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_WhileWithContinue)
{
    std::string code = R"(
void F()
{
    create i32 %i;
    create i32 %sum;
    mov %i, 0;
    mov %sum, 0;
    while (%i LT 20)
    {
        add %i, 1;
        if (%i EQ 10)
        {
            continue;
        }
        add %sum, %i;
    }
    nop;
})";
    EXPECT_TRUE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_MultipleLabels)
{
    std::string code = R"(
void F()
{
    label_a:
    {
        create i32 %x;
        mov %x, 1;
        nop;
    }
    label_b:
    {
        create i32 %y;
        mov %y, 2;
        nop;
    }
    label_c:
    {
        create i32 %z;
        mov %z, 3;
        nop;
    }
})";
    EXPECT_TRUE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_IfElseIfElseChain)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    mov %x, 42;

    if (%x EQ 10)
    {
        mov %x, 0;
    }
    else if (%x EQ 20)
    {
        mov %x, 1;
    }
    else if (%x EQ 30)
    {
        mov %x, 2;
    }
    else
    {
        mov %x, 3;
    }
    nop;
})";
    EXPECT_TRUE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_ManyParameters)
{
    std::string code = R"(
void F(i32 %a, i32 %b, i32 %c, i32 %d, i64 %e, i64 %f)
{
    create i64 %sum;
    mov %sum, 0;
    add %sum, 1;
    nop;
})";
    EXPECT_TRUE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_VariableReusedInExpressions)
{
    std::string code = R"(
void F()
{
    create i32 %x;
    mov %x, 1;
    add %x, 2;
    sub %x, 3;
    and %x, 0xFF;
    or  %x, 0x10;
    xor %x, 0xAA;
    shl %x, 4;
    shr %x, 2;
    nop;
})";
    EXPECT_TRUE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_AllComparisonOperators)
{
    std::string code = R"(
void F()
{
    create i32 %a;
    create i32 %b;
    mov %a, 10;
    mov %b, 20;

    if (%a EQ %b) { nop; }
    if (%a LT %b) { nop; }
    if (%a GT %b) { nop; }
    nop;
})";
    EXPECT_TRUE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_VariablesInLabelScope)
{
    std::string code = R"(
void F()
{
    myLabel:
    {
        create i32 %scopedX;
        mov %scopedX, 99;
        nop;
    }
    nop;
})";
    EXPECT_TRUE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_ParamUsedDirectly)
{
    std::string code = R"(
void F(i32 %x)
{
    add %x, 10;
    nop;
})";
    EXPECT_TRUE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_ParamCopiedToLocal)
{
    std::string code = R"(
void F(i32 %param)
{
    create i32 %local;
    mov %local, %param;
    nop;
})";
    EXPECT_TRUE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_HexImmediates)
{
    std::string code = R"(
void F()
{
    create i64 %x;
    mov %x, 0xDEADBEEF;
    and %x, 0xFF00FF00;
    or  %x, 0x00FF00FF;
    xor %x, 0xCAFEBABE;
    nop;
})";
    EXPECT_TRUE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_MemoryAccessWithOffset)
{
    std::string code = R"(
void F()
{
    create i64 %base;
    create i64 %val1;
    create i64 %val2;
    create i64 %val3;
    mov %base, 0;
    mov %val1, i64 (%base+0);
    mov %val2, i64 (%base+0x10);
    mov %val3, i64 (%base+0xFF);
    nop;
})";
    EXPECT_TRUE(compileFromSource(code));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_ModuleWithExternalRef)
{
    EXPECT_TRUE(compileFromSource("include <\"includable_module.ez\"> void F() { call i32 %IncludableHelper(); }"));
}

TEST_F(FrontendCompilerTestFixture, CompilerDriver_Compile_ModuleWithInvalidExternalRef)
{
    EXPECT_FALSE(compileFromSource("include <\"includable_module.ez\"> void F() { call i32 %NotIncludableHelper(); }"));
}