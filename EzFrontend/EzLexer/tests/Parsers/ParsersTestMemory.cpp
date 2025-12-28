#include "ParsersTestFixture.h"

TEST_F(ParsersTestFixture, BaseDispl)
{
    std::string input = R"(i8 (%myVar, 1231))";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<MemoryOperandParser>());
    TEST_MEMORY(MemoryOperandType::BaseDisplacement, "i8");
    TEST_MEMORY_BASE_DISPL(AstNodeType::Variable, AstNodeType::Immediate);
}

TEST_F(ParsersTestFixture, BaseDisplInvalidBase)
{
    std::string input = R"(i8 (1231, 1231))";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<MemoryOperandParser>());
}

TEST_F(ParsersTestFixture, BaseDisplInvalidDispl)
{
    std::string input = R"(i8 (%base, %wrong))";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<MemoryOperandParser>());
}

TEST_F(ParsersTestFixture, BaseIndexScaleDisplacement)
{
    // 4 = scaling factor, 1231 = displacement.
    std::string input = R"(i8 (%base, %index, 4, 1231))";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<MemoryOperandParser>());
    TEST_MEMORY(MemoryOperandType::BaseIndexScaleDisplacement, "i8");
    TEST_MEMORY_BASE_INDEX_SCALE_DISPL(AstNodeType::Variable,
                                       AstNodeType::Variable,
                                       AstNodeType::Immediate,
                                       AstNodeType::Immediate);
}

TEST_F(ParsersTestFixture, BaseIndexScaleDisplacementInvalidBase)
{
    // myVar = index, 1231 = scaling factor
    std::string input = R"(i8 (1231, %index, 4, 1231))";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<MemoryOperandParser>());
}

TEST_F(ParsersTestFixture, BaseIndexScaleDisplacementInvalidIndex)
{
    // myVar = index, 1231 = scaling factor
    std::string input = R"(i8 (%base, 1231, 4, 1231))";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<MemoryOperandParser>());
}

TEST_F(ParsersTestFixture, BaseIndexScaleDisplacementInvalidScale)
{
    // myVar = index, 1231 = scaling factor
    std::string input = R"(i8 (%base, %index, %wrong, 1231))";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<MemoryOperandParser>());
}

TEST_F(ParsersTestFixture, BaseIndexScaleInvalidDispl)
{
    // myVar = index, 1231 = scaling factor
    std::string input = R"(i8 (%base, %index, 4, %wrong))";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<MemoryOperandParser>());
}

TEST_F(ParsersTestFixture, IndexScale)
{
    // myVar = index, 1231 = scaling factor
    std::string input = R"(i8 (, %myVar, 4))";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<MemoryOperandParser>());
    TEST_MEMORY(MemoryOperandType::IndexScale, "i8");
    TEST_MEMORY_INDEX_SCALE(AstNodeType::Variable, AstNodeType::Immediate);
}

TEST_F(ParsersTestFixture, IndexScaleInvalidIndex)
{
    // myVar = index, 1231 = scaling factor
    std::string input = R"(i8 (, 231, 4))";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<MemoryOperandParser>());
}

TEST_F(ParsersTestFixture, IndexScaleInvalidScale)
{
    // myVar = index, 1231 = scaling factor
    std::string input = R"(i8 (, %myVar, %wrong))";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<MemoryOperandParser>());
}

TEST_F(ParsersTestFixture, Direct)
{
    std::string input = R"(i8 (1231))";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<MemoryOperandParser>());
    TEST_MEMORY(MemoryOperandType::Direct, "i8");
    TEST_MEMORY_DIRECT();
}