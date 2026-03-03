#include "ParsersTestFixture.h"

// =============================================================================
//  Module Header – valid
// =============================================================================

TEST_F(ParsersTestFixture, ModuleHeader_NoParams)
{
    EXPECT_TRUE(tokenizeAndParse<ModuleParser::ModuleHeaderParser>("i8 myModule()"));
    TEST_MODULE_HEADER("myModule", "i8");
}

TEST_F(ParsersTestFixture, ModuleHeader_VoidReturn)
{
    EXPECT_TRUE(tokenizeAndParse<ModuleParser::ModuleHeaderParser>("void myModule()"));
    TEST_MODULE_HEADER("myModule", "void");
}

TEST_F(ParsersTestFixture, ModuleHeader_1Param)
{
    EXPECT_TRUE(tokenizeAndParse<ModuleParser::ModuleHeaderParser>("i8 myModule(i8 %myParam)"));
    TEST_MODULE_HEADER("myModule", "i8", AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, ModuleHeader_2Params)
{
    EXPECT_TRUE(tokenizeAndParse<ModuleParser::ModuleHeaderParser>("i8 myModule(i8 %myParam, i64 %myParam2)"));
    TEST_MODULE_HEADER("myModule", "i8", AstNodeType::Variable, AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, ModuleHeader_3Params)
{
    EXPECT_TRUE(tokenizeAndParse<ModuleParser::ModuleHeaderParser>("i64 func(i32 %a, i64 %b, i8 %c)"));
    TEST_MODULE_HEADER("func", "i64", AstNodeType::Variable, AstNodeType::Variable, AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, ModuleHeader_i64Return)
{
    EXPECT_TRUE(tokenizeAndParse<ModuleParser::ModuleHeaderParser>("i64 compute(i32 %n)"));
    TEST_MODULE_HEADER("compute", "i64", AstNodeType::Variable);
}

// =============================================================================
//  Module Header – invalid
// =============================================================================

TEST_F(ParsersTestFixture, ModuleHeader_InvalidType)
{
    EXPECT_FALSE(tokenizeAndParse<ModuleParser::ModuleHeaderParser>("123 myModule(i8 %myParam)"));
}

TEST_F(ParsersTestFixture, ModuleHeader_InvalidName)
{
    EXPECT_FALSE(tokenizeAndParse<ModuleParser::ModuleHeaderParser>("i8 123(i8 %myParam)"));
}

TEST_F(ParsersTestFixture, ModuleHeader_InvalidParamType)
{
    EXPECT_FALSE(tokenizeAndParse<ModuleParser::ModuleHeaderParser>("i8 myModule(123 %myParam)"));
}

TEST_F(ParsersTestFixture, ModuleHeader_MissingLeftParen)
{
    EXPECT_FALSE(tokenizeAndParse<ModuleParser::ModuleHeaderParser>("i8 myModule i8 %myParam)"));
}

TEST_F(ParsersTestFixture, ModuleHeader_MissingRightParen)
{
    EXPECT_FALSE(tokenizeAndParse<ModuleParser::ModuleHeaderParser>("i8 myModule(i8 %myParam"));
}

// =============================================================================
//  Code Scope (Module Body) – valid
// =============================================================================

TEST_F(ParsersTestFixture, CodeScope_Empty)
{
    EXPECT_TRUE(tokenizeAndParse<CodeScopeParser>("{}"));
}

TEST_F(ParsersTestFixture, CodeScope_1Instruction)
{
    std::string input = R"(
{
    add i8 %myVar, 1;
})";
    EXPECT_TRUE(tokenizeAndParse<CodeScopeParser>(input));
}

TEST_F(ParsersTestFixture, CodeScope_ManyInstructions)
{
    std::string input = R"(
{
    add i8 %myVar, 1;
    add %myVar2, i64 (1234);
    add %myVar3, i64 (%base+0);
    add %myVar4, i64 (%base+0xFEEF);
    nop;
})";
    EXPECT_TRUE(tokenizeAndParse<CodeScopeParser>(input));
}

TEST_F(ParsersTestFixture, CodeScope_WithLabel)
{
    std::string input = R"(
{
    add %myVar, 1;
    myLabel: {}
})";
    EXPECT_TRUE(tokenizeAndParse<CodeScopeParser>(input));
}

TEST_F(ParsersTestFixture, CodeScope_WithMultipleLabels)
{
    std::string input = R"(
{
    add i8 %myVar, 1;
    myLabel:
    {
        add i8 %myVar, 1;
    }
    myLabel2:
    {
        add %myVar2, i64 (1234);
        add %myVar3, i32 (%base+0);
        add %myVar4, i16 (%base+0xFEEF);
    }
})";
    EXPECT_TRUE(tokenizeAndParse<CodeScopeParser>(input));
}

TEST_F(ParsersTestFixture, CodeScope_NestedLabel)
{
    std::string input = R"(
{
    add i8 %myVar, 1;
    myLabel:
    {
        add i8 %myVar, 1;
        myLabel2:
        {
        add %myVar2, i64 (1234);
        add %myVar3, i32 (%base+0);
        add %myVar4, i16 (%base+0xFEEF);
        }
    }
})";
    EXPECT_TRUE(tokenizeAndParse<CodeScopeParser>(input));
}

TEST_F(ParsersTestFixture, CodeScope_WithIf)
{
    std::string input = R"(
{
    if (%a EQ %b) { nop; }
})";
    EXPECT_TRUE(tokenizeAndParse<CodeScopeParser>(input));
}

TEST_F(ParsersTestFixture, CodeScope_WithWhile)
{
    std::string input = R"(
{
    while (%x LT %y) { nop; }
})";
    EXPECT_TRUE(tokenizeAndParse<CodeScopeParser>(input));
}

TEST_F(ParsersTestFixture, CodeScope_WithIfWhileLabel)
{
    std::string input = R"(
{
    nop;
    if (%a EQ %b) { nop; } else { nop; }
    while (%x LT %y) { add %x, 1; }
    myLabel: { nop; }
})";
    EXPECT_TRUE(tokenizeAndParse<CodeScopeParser>(input));
}

// =============================================================================
//  Code Scope – invalid
// =============================================================================

TEST_F(ParsersTestFixture, CodeScope_MissingLeftBrace)
{
    EXPECT_FALSE(tokenizeAndParse<CodeScopeParser>("add i8 %myVar, 1; }"));
}

TEST_F(ParsersTestFixture, CodeScope_MissingRightBrace)
{
    EXPECT_FALSE(tokenizeAndParse<CodeScopeParser>("{ add i8 %myVar, 1;"));
}

// =============================================================================
//  Full Module (header + body) – valid
// =============================================================================

TEST_F(ParsersTestFixture, Module_SimpleVoid)
{
    std::string input = R"(
void MyFunc()
{
    nop;
})";
    EXPECT_TRUE(tokenizeAndParse<ModuleParser::ModuleParser>(input));

    Module *module;
    ASSERT_TRUE(expectNodeCast<>(module));
    EXPECT_NE(module->getHeader(), nullptr);
    EXPECT_NE(module->getBody(), nullptr);
    EXPECT_EQ(module->getHeader()->getModuleName(), "MyFunc");
    EXPECT_EQ(module->getHeader()->getReturnTypeName(), "void");
}

TEST_F(ParsersTestFixture, Module_WithParamsAndBody)
{
    std::string input = R"(
i64 Compute(i32 %a, i64 %b)
{
    create i32 %local;
    add %local, 1;
    nop;
})";
    EXPECT_TRUE(tokenizeAndParse<ModuleParser::ModuleParser>(input));

    Module *module;
    ASSERT_TRUE(expectNodeCast<>(module));
    EXPECT_EQ(module->getHeader()->getReturnTypeName(), "i64");
    EXPECT_EQ(module->getHeader()->getExpressions()->m_numElems, 2);
    EXPECT_GE(module->getBody()->getExpressions()->m_numElems, 3);
}

TEST_F(ParsersTestFixture, Module_WithIfAndWhile)
{
    std::string input = R"(
i64 MyFunc(i32 %x, i32 %y)
{
    if (%x EQ %y) { nop; }
    while (%x LT %y) { add %x, 1; }
    nop;
})";
    EXPECT_TRUE(tokenizeAndParse<ModuleParser::ModuleParser>(input));

    Module *module;
    ASSERT_TRUE(expectNodeCast<>(module));
    auto exprs = module->getBody()->getExpressions();
    EXPECT_EQ(exprs->m_numElems, 3); // if, while, nop
}

TEST_F(ParsersTestFixture, Module_WithLabels)
{
    std::string input = R"(
void ModWithLabels()
{
    nop;
    lbl1: { nop; }
    lbl2: { nop; }
})";
    EXPECT_TRUE(tokenizeAndParse<ModuleParser::ModuleParser>(input));

    Module *module;
    ASSERT_TRUE(expectNodeCast<>(module));
    auto exprs = module->getBody()->getExpressions();
    EXPECT_EQ(exprs->m_numElems, 3); // nop, lbl1, lbl2
}

TEST_F(ParsersTestFixture, Module_ComplexProgram)
{
    std::string input = R"(
i64 CalculateChecksum(i64 %bufferPtr, i32 %length, i64 %key)
{
    create i64 %runningSum;
    create i32 %counter;
    create i64 %currentAddr;

    mov %runningSum, 0;
    mov %counter, 0;
    mov %currentAddr, %bufferPtr;

    while (%counter LT %length)
    {
        add %runningSum, 1;
        add %currentAddr, 1;
        add %counter, 1;
    }

    label_end:
    {
        nop;
    }
})";
    EXPECT_TRUE(tokenizeAndParse<ModuleParser::ModuleParser>(input));
}
