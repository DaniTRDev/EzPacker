#include "ParsersTestFixture.h"

// =============================================================================
//  Global Variables – single initializer
// =============================================================================

TEST_F(ParsersTestFixture, GlobalVariable_SingleIntInit)
{
    EXPECT_TRUE(tokenizeAndParse<VariableParser>("i8 %myVar: 12345"));
    TEST_VARIABLE(false, "i8", "myVar", AstNodeType::Immediate);
}

TEST_F(ParsersTestFixture, GlobalVariable_SingleHexInit)
{
    EXPECT_TRUE(tokenizeAndParse<VariableParser>("i32 %addr: 0xDEAD"));
    TEST_VARIABLE(false, "i32", "addr", AstNodeType::Immediate);
}

TEST_F(ParsersTestFixture, GlobalVariable_InvalidMissingInit)
{
    EXPECT_FALSE(tokenizeAndParse<VariableParser>("i8 %myVar: "));
}

// =============================================================================
//  Global Variables – array initializer
// =============================================================================

TEST_F(ParsersTestFixture, GlobalVariable_Array3Elems)
{
    EXPECT_TRUE(tokenizeAndParse<VariableParser>("i8 %myVar: {1234, 1231, 0xFFFFFF}"));
    TEST_VARIABLE(true, "i8", "myVar", AstNodeType::Immediate, AstNodeType::Immediate, AstNodeType::Immediate);
}

TEST_F(ParsersTestFixture, GlobalVariable_SingleElementWithBraces)
{
    EXPECT_TRUE(tokenizeAndParse<VariableParser>("i32 %single: {42}"));
    TEST_VARIABLE(false, "i32", "single", AstNodeType::Immediate);
}

TEST_F(ParsersTestFixture, GlobalVariable_InvalidArrayUnclosed)
{
    EXPECT_FALSE(tokenizeAndParse<VariableParser>("i8 %myVar: {"));
}

TEST_F(ParsersTestFixture, GlobalVariable_InvalidArrayMissingRBrace)
{
    EXPECT_FALSE(tokenizeAndParse<VariableParser>("i8 %myVar: { 1, 2 "));
}

// =============================================================================
//  Local Variables (no type, no initializer)
// =============================================================================

TEST_F(ParsersTestFixture, LocalVariable_Simple)
{
    EXPECT_TRUE(tokenizeAndParse<VariableParser>("%myVar"));
    TEST_VARIABLE(false, "", "myVar", );
}

TEST_F(ParsersTestFixture, LocalVariable_WithType)
{
    EXPECT_TRUE(tokenizeAndParse<VariableParser>("i64 %bigVar"));
    TEST_VARIABLE(false, "i64", "bigVar", );
}

// =============================================================================
//  Variable error cases
// =============================================================================

TEST_F(ParsersTestFixture, Variable_MissingPercent) { EXPECT_FALSE(tokenizeAndParse<VariableParser>("i8 myVar")); }

TEST_F(ParsersTestFixture, Variable_MissingName) { EXPECT_FALSE(tokenizeAndParse<VariableParser>("i8 %")); }

TEST_F(ParsersTestFixture, Variable_TypeOnly)
{
    // "i8" alone is an identifier, not a variable
    EXPECT_FALSE(tokenizeAndParse<VariableParser>("i8"));
}
