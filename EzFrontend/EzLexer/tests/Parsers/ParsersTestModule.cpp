#include "ParsersTestFixture.h"

TEST_F(ParsersTestFixture, ModuleHeaderNoParams)
{
    std::string input = "i8 myModule()";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<ModuleParser::ModuleHeaderParser>());
    TEST_MODULE_HEADER("myModule", "i8");
}

TEST_F(ParsersTestFixture, ModuleHeaderInvalidType)
{
    std::string input = "123 myModule(i8 %myParam)";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<ModuleParser::ModuleHeaderParser>());
}

TEST_F(ParsersTestFixture, ModuleHeaderInvalidName)
{
    std::string input = "i8 123(i8 %myParam)";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<ModuleParser::ModuleHeaderParser>());
}

TEST_F(ParsersTestFixture, ModuleHeader1Param)
{
    std::string input = "i8 myModule(i8 %myParam)";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<ModuleParser::ModuleHeaderParser>());
    TEST_MODULE_HEADER("myModule", "i8", AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, ModuleHeader1ParamInvalidType)
{
    std::string input = "i8 myModule(123 %myParam)";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<ModuleParser::ModuleHeaderParser>());
}

TEST_F(ParsersTestFixture, ModuleHeader2Params)
{
    std::string input = "i8 myModule(i8 %myParam, i64 %myParam2)";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<ModuleParser::ModuleHeaderParser>());
    TEST_MODULE_HEADER("myModule", "i8", AstNodeType::Variable, AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, ModuleHeaderMissingLeftParen)
{
    std::string input = "i8 myModule i8 %myParam)";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<ModuleParser::ModuleHeaderParser>());
}

TEST_F(ParsersTestFixture, ModuleHeaderMissingRightParen)
{
    std::string input = "i8 myModule(i8 %myParam";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<ModuleParser::ModuleHeaderParser>());
}

TEST_F(ParsersTestFixture, ModuleBodyEmpty)
{
    std::string input = "{}";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<CodeScopeParser>());
}

TEST_F(ParsersTestFixture, ModuleBody1Instr)
{
    std::string input = R"(
{
    add i8 %myVar, 1;
}
)";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<CodeScopeParser>());
}

TEST_F(ParsersTestFixture, ModuleBodyNInstr)
{
    std::string input = R"(
{
    add i8 %myVar, 1;
    add %myVar2, i64 (1234);
    add %myVar3, i64 (%base+0);
    add %myVar4, i64 (%base+0xFEEF);
    nop;
}
)";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<CodeScopeParser>());
}

TEST_F(ParsersTestFixture, ModuleBody1Instr1Label)
{
    std::string input = R"(
{
    add %myVar, 1;
    myLabel: {}
}
)";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<CodeScopeParser>());
}

TEST_F(ParsersTestFixture, ModuleBody1Instr1LabelBounds)
{
    std::string input = R"(
{
    add i8 %myVar, 1;
    myLabel:
    {
        add i8 %myVar, 1;
    }
    myLabel: {}
}
)";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<CodeScopeParser>());
}

TEST_F(ParsersTestFixture, ModuleBodyNInstrNLabelBounds)
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
}
)";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<CodeScopeParser>());
}

TEST_F(ParsersTestFixture, ModuleBodyNInstrNestedLabel)
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
}
)";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<CodeScopeParser>());
}

TEST_F(ParsersTestFixture, ModuleBodyMissingLeftBrace)
{
    std::string input = "add i8 %myVar, 1; }";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<CodeScopeParser>());
}

TEST_F(ParsersTestFixture, ModuleBodyMissingRightBrace)
{
    std::string input = "{ add i8 %myVar, 1;";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<CodeScopeParser>());
}
