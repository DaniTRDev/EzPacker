#include "ParsersTestFixture.h"

// =============================================================================
//  Integer Immediates
// =============================================================================

TEST_F(ParsersTestFixture, ImmediateInteger_Zero)
{
    EXPECT_TRUE(tokenizeAndParse<ImmediateParser::ImmediateParser>("0"));
    TEST_INTEGER_IMMEDIATE((uint64_t)0);
}

TEST_F(ParsersTestFixture, ImmediateInteger_One)
{
    EXPECT_TRUE(tokenizeAndParse<ImmediateParser::ImmediateParser>("1"));
    TEST_INTEGER_IMMEDIATE((uint64_t)1);
}

TEST_F(ParsersTestFixture, ImmediateInteger_U8Max)
{
    tokenizeAndCreateContext(std::format("{}", UINT8_MAX));
    EXPECT_TRUE(expectParse<ImmediateParser::ImmediateParser>());
    TEST_INTEGER_IMMEDIATE(UINT8_MAX);
}

TEST_F(ParsersTestFixture, ImmediateInteger_U16Max)
{
    tokenizeAndCreateContext(std::format("{}", UINT16_MAX));
    EXPECT_TRUE(expectParse<ImmediateParser::ImmediateParser>());
    TEST_INTEGER_IMMEDIATE(UINT16_MAX);
}

TEST_F(ParsersTestFixture, ImmediateInteger_U32Max)
{
    tokenizeAndCreateContext(std::format("{}", UINT32_MAX));
    EXPECT_TRUE(expectParse<ImmediateParser::ImmediateParser>());
    TEST_INTEGER_IMMEDIATE(UINT32_MAX);
}

TEST_F(ParsersTestFixture, ImmediateInteger_U64Max)
{
    tokenizeAndCreateContext(std::format("{}", UINT64_MAX));
    EXPECT_TRUE(expectParse<ImmediateParser::ImmediateParser>());
    TEST_INTEGER_IMMEDIATE(UINT64_MAX);
}

TEST_F(ParsersTestFixture, ImmediateInteger_128bit)
{
    mp_int int128;
    EXPECT_EQ(mp_init(&int128), MP_OKAY);
    uint64_t words[2] = { UINT64_MAX, UINT64_MAX };
    EXPECT_EQ(mp_unpack(&int128, 2, -1, sizeof(uint64_t), 0, 0, words), MP_OKAY);

    tokenizeAndCreateContext(std::format("0x{:X}{:X}", words[0], words[1]));
    EXPECT_TRUE(expectParse<ImmediateParser::ImmediateParser>());
    TEST_BIG_INTEGER_IMMEDIATE(&int128);
}

TEST_F(ParsersTestFixture, ImmediateInteger_Hex)
{
    EXPECT_TRUE(tokenizeAndParse<ImmediateParser::ImmediateParser>("0xFF"));

    ImmediateOperand *imm;
    EXPECT_TRUE(expectNodeCast(imm));
    EXPECT_EQ(imm->getImmediateType(), ImmediateType::Integer);
}

TEST_F(ParsersTestFixture, ImmediateInteger_HexUppercase)
{
    EXPECT_TRUE(tokenizeAndParse<ImmediateParser::ImmediateParser>("0xDEADBEEF"));

    ImmediateOperand *imm;
    EXPECT_TRUE(expectNodeCast(imm));
    EXPECT_EQ(imm->getImmediateType(), ImmediateType::Integer);
}

TEST_F(ParsersTestFixture, ImmediateInteger_HexMixed)
{
    EXPECT_TRUE(tokenizeAndParse<ImmediateParser::ImmediateParser>("0xAbCd1234"));

    ImmediateOperand *imm;
    EXPECT_TRUE(expectNodeCast(imm));
    EXPECT_EQ(imm->getImmediateType(), ImmediateType::Integer);
}

TEST_F(ParsersTestFixture, ImmediateInteger_InvalidHex)
{
    EXPECT_FALSE(tokenizeAndParse<ImmediateParser::ImmediateParser>("0xZZ"));
}

// =============================================================================
//  Floating-point Immediates
// =============================================================================

TEST_F(ParsersTestFixture, ImmediateFloat_Pi)
{
    tokenizeAndCreateContext(std::format("{}", 3.141516));
    EXPECT_TRUE(expectParse<ImmediateParser::ImmediateParser>());
    TEST_FLOAT_IMMEDIATE(3.141516);
}

TEST_F(ParsersTestFixture, ImmediateFloat_Zero)
{
    tokenizeAndCreateContext(std::format("{:.1f}", 0.f));
    EXPECT_TRUE(expectParse<ImmediateParser::ImmediateParser>());
    TEST_FLOAT_IMMEDIATE(0.f);
}

TEST_F(ParsersTestFixture, ImmediateFloat_SmallFraction)
{
    tokenizeAndCreateContext("0.001");
    EXPECT_TRUE(expectParse<ImmediateParser::ImmediateParser>());
    TEST_FLOAT_IMMEDIATE(0.001);
}

TEST_F(ParsersTestFixture, ImmediateFloat_Large)
{
    tokenizeAndCreateContext("99999.99999");
    EXPECT_TRUE(expectParse<ImmediateParser::ImmediateParser>());
    TEST_FLOAT_IMMEDIATE(99999.99999);
}

TEST_F(ParsersTestFixture, ImmediateFloat_InvalidDoubleDot)
{
    EXPECT_FALSE(tokenizeAndParse<ImmediateParser::ImmediateParser>("3.14.15"));
}

// =============================================================================
//  String Immediates
// =============================================================================

TEST_F(ParsersTestFixture, ImmediateString_Basic)
{
    EXPECT_TRUE(tokenizeAndParse<ImmediateParser::ImmediateParser>("\"TEST_STRING\""));
    TEST_STRING_IMMEDIATE("TEST_STRING");
}

TEST_F(ParsersTestFixture, ImmediateString_Empty)
{
    EXPECT_TRUE(tokenizeAndParse<ImmediateParser::ImmediateParser>("\"\""));
    TEST_STRING_IMMEDIATE("");
}

TEST_F(ParsersTestFixture, ImmediateString_WithSpaces)
{
    EXPECT_TRUE(tokenizeAndParse<ImmediateParser::ImmediateParser>("\"hello world\""));
    TEST_STRING_IMMEDIATE("hello world");
}

TEST_F(ParsersTestFixture, ImmediateString_SingleChar)
{
    EXPECT_TRUE(tokenizeAndParse<ImmediateParser::ImmediateParser>("\"A\""));
    TEST_STRING_IMMEDIATE("A");
}
