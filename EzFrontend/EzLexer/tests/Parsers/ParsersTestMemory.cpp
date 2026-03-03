#include "ParsersTestFixture.h"

// =============================================================================
//  BaseDisplacement  – valid
// =============================================================================

TEST_F(ParsersTestFixture, Memory_BaseDispl_Subtract)
{
    EXPECT_TRUE(tokenizeAndParse<MemoryOperandParser::MemoryOperandParser>("i8 (%myVar-1231)"));
    TEST_MEMORY(MemoryOperandType::BaseDisplacement, "i8");
    TEST_MEMORY_BASE_DISPL(AstNodeType::Variable, AstNodeType::Immediate);
}

TEST_F(ParsersTestFixture, Memory_BaseDispl_Add)
{
    EXPECT_TRUE(tokenizeAndParse<MemoryOperandParser::MemoryOperandParser>("i64 (%base+0x10)"));
    TEST_MEMORY(MemoryOperandType::BaseDisplacement, "i64");
    TEST_MEMORY_BASE_DISPL(AstNodeType::Variable, AstNodeType::Immediate);
}

TEST_F(ParsersTestFixture, Memory_BaseDispl_ZeroOffset)
{
    EXPECT_TRUE(tokenizeAndParse<MemoryOperandParser::MemoryOperandParser>("i32 (%ptr+0)"));
    TEST_MEMORY(MemoryOperandType::BaseDisplacement, "i32");
    TEST_MEMORY_BASE_DISPL(AstNodeType::Variable, AstNodeType::Immediate);
}

TEST_F(ParsersTestFixture, Memory_BaseDispl_Whitespace)
{
    EXPECT_TRUE(tokenizeAndParse<MemoryOperandParser::MemoryOperandParser>("i16 (%base      +       123)"));
    TEST_MEMORY(MemoryOperandType::BaseDisplacement, "i16");
    TEST_MEMORY_BASE_DISPL(AstNodeType::Variable, AstNodeType::Immediate);
}

// =============================================================================
//  BaseDisplacement  – invalid
// =============================================================================

TEST_F(ParsersTestFixture, Memory_BaseDispl_InvalidBase)
{
    EXPECT_FALSE(tokenizeAndParse<MemoryOperandParser::MemoryOperandParser>("i8 (1231+1231)"));
}

TEST_F(ParsersTestFixture, Memory_BaseDispl_InvalidDispl)
{
    EXPECT_FALSE(tokenizeAndParse<MemoryOperandParser::MemoryOperandParser>("i8 (%base + %wrong)"));
}

// =============================================================================
//  BaseIndexScaleDisplacement – valid
// =============================================================================

TEST_F(ParsersTestFixture, Memory_BISD_Valid)
{
    EXPECT_TRUE(tokenizeAndParse<MemoryOperandParser::MemoryOperandParser>("i8 (%base, %index, 4, 1231)"));
    TEST_MEMORY(MemoryOperandType::BaseIndexScaleDisplacement, "i8");
    TEST_MEMORY_BASE_INDEX_SCALE_DISPL(AstNodeType::Variable,
                                       AstNodeType::Variable,
                                       AstNodeType::Immediate,
                                       AstNodeType::Immediate);
}

// =============================================================================
//  BaseIndexScaleDisplacement – invalid
// =============================================================================

TEST_F(ParsersTestFixture, Memory_BISD_InvalidBase)
{
    EXPECT_FALSE(tokenizeAndParse<MemoryOperandParser::MemoryOperandParser>("i8 (1231, %index, 4, 1231)"));
}

TEST_F(ParsersTestFixture, Memory_BISD_InvalidIndex)
{
    EXPECT_FALSE(tokenizeAndParse<MemoryOperandParser::MemoryOperandParser>("i8 (%base, 1231, 4, 1231)"));
}

TEST_F(ParsersTestFixture, Memory_BISD_InvalidScale)
{
    EXPECT_FALSE(tokenizeAndParse<MemoryOperandParser::MemoryOperandParser>("i8 (%base, %index, %wrong, 1231)"));
}

TEST_F(ParsersTestFixture, Memory_BISD_InvalidDispl)
{
    EXPECT_FALSE(tokenizeAndParse<MemoryOperandParser::MemoryOperandParser>("i8 (%base, %index, 4, %wrong)"));
}

// =============================================================================
//  IndexScale – valid
// =============================================================================

TEST_F(ParsersTestFixture, Memory_IndexScale_Valid)
{
    EXPECT_TRUE(tokenizeAndParse<MemoryOperandParser::MemoryOperandParser>("i8 (, %myVar, 4)"));
    TEST_MEMORY(MemoryOperandType::IndexScale, "i8");
    TEST_MEMORY_INDEX_SCALE(AstNodeType::Variable, AstNodeType::Immediate);
}

TEST_F(ParsersTestFixture, Memory_IndexScale_Scale1)
{
    EXPECT_TRUE(tokenizeAndParse<MemoryOperandParser::MemoryOperandParser>("i64 (, %idx, 1)"));
    TEST_MEMORY(MemoryOperandType::IndexScale, "i64");
    TEST_MEMORY_INDEX_SCALE(AstNodeType::Variable, AstNodeType::Immediate);
}

// =============================================================================
//  IndexScale – invalid
// =============================================================================

TEST_F(ParsersTestFixture, Memory_IndexScale_InvalidIndex)
{
    EXPECT_FALSE(tokenizeAndParse<MemoryOperandParser::MemoryOperandParser>("i8 (, 231, 4)"));
}

TEST_F(ParsersTestFixture, Memory_IndexScale_InvalidScale)
{
    EXPECT_FALSE(tokenizeAndParse<MemoryOperandParser::MemoryOperandParser>("i8 (, %myVar, %wrong)"));
}

// =============================================================================
//  Direct – valid
// =============================================================================

TEST_F(ParsersTestFixture, Memory_Direct_Decimal)
{
    EXPECT_TRUE(tokenizeAndParse<MemoryOperandParser::MemoryOperandParser>("i8 (1231)"));
    TEST_MEMORY(MemoryOperandType::Direct, "i8");
    TEST_MEMORY_DIRECT();
}

TEST_F(ParsersTestFixture, Memory_Direct_Hex)
{
    EXPECT_TRUE(tokenizeAndParse<MemoryOperandParser::MemoryOperandParser>("i64 (0x400000)"));
    TEST_MEMORY(MemoryOperandType::Direct, "i64");
    TEST_MEMORY_DIRECT();
}

// =============================================================================
//  Structural error cases
// =============================================================================

TEST_F(ParsersTestFixture, Memory_MissingLeftParen)
{
    EXPECT_FALSE(tokenizeAndParse<MemoryOperandParser::MemoryOperandParser>("i8 1231)"));
}

TEST_F(ParsersTestFixture, Memory_MissingRightParen)
{
    EXPECT_FALSE(tokenizeAndParse<MemoryOperandParser::MemoryOperandParser>("i8 (1231"));
}

TEST_F(ParsersTestFixture, Memory_InvalidContent)
{
    EXPECT_FALSE(tokenizeAndParse<MemoryOperandParser::MemoryOperandParser>("i8 (invalid)"));
}

TEST_F(ParsersTestFixture, Memory_EmptyParens)
{
    EXPECT_FALSE(tokenizeAndParse<MemoryOperandParser::MemoryOperandParser>("i8 ()"));
}
