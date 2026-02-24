#include "ParsersTestFixture.h"

TEST_F(ParsersTestFixture, ImmediateInteger8)
{
    tokenizeAndCreateContext(std::format("{}", UINT8_MAX));
    EXPECT_TRUE(expectParse<ImmediateParser::ImmediateParser>());

    std::shared_ptr<ImmediateOperand> imm;
    EXPECT_TRUE(expectNodeCast(imm));

    TEST_INTEGER_IMMEDIATE(UINT8_MAX);
}

TEST_F(ParsersTestFixture, ImmediateInteger16)
{
    tokenizeAndCreateContext(std::format("{}", UINT16_MAX));
    EXPECT_TRUE(expectParse<ImmediateParser::ImmediateParser>());

    std::shared_ptr<ImmediateOperand> imm;
    EXPECT_TRUE(expectNodeCast<>(imm));

    TEST_INTEGER_IMMEDIATE(UINT16_MAX);
}

TEST_F(ParsersTestFixture, ImmediateInteger32)
{
    tokenizeAndCreateContext(std::format("{}", UINT32_MAX));
    EXPECT_TRUE(expectParse<ImmediateParser::ImmediateParser>());

    std::shared_ptr<ImmediateOperand> imm;
    EXPECT_TRUE(expectNodeCast<>(imm));

    TEST_INTEGER_IMMEDIATE(UINT32_MAX);
}

TEST_F(ParsersTestFixture, ImmediateInteger64)
{
    tokenizeAndCreateContext(std::format("{}", UINT64_MAX));
    EXPECT_TRUE(expectParse<ImmediateParser::ImmediateParser>());

    std::shared_ptr<ImmediateOperand> imm;
    EXPECT_TRUE(expectNodeCast(imm));

    TEST_INTEGER_IMMEDIATE(UINT64_MAX);
}

// Uses hex format for the input number (easier testing).
TEST_F(ParsersTestFixture, ImmediateInteger128)
{
    mp_int int128;
    EXPECT_EQ(mp_init(&int128), MP_OKAY);

    uint64_t words[2] = { UINT64_MAX, UINT64_MAX };
    EXPECT_EQ(mp_unpack(&int128, 2, -1, sizeof(uint64_t), 0, 0, words), MP_OKAY);

    tokenizeAndCreateContext(std::format("0x{:X}{:X}", words[0], words[1]));
    EXPECT_TRUE(expectParse<ImmediateParser::ImmediateParser>());

    std::shared_ptr<ImmediateOperand> imm;
    EXPECT_TRUE(expectNodeCast(imm));

    TEST_BIG_INTEGER_IMMEDIATE(&int128);
}

TEST_F(ParsersTestFixture, ImmediateFloat)
{
    tokenizeAndCreateContext(std::format("{}", 3.141516));
    EXPECT_TRUE(expectParse<ImmediateParser::ImmediateParser>());

    std::shared_ptr<ImmediateOperand> imm;
    EXPECT_TRUE(expectNodeCast(imm));

    TEST_FLOAT_IMMEDIATE(3.141516);
}

TEST_F(ParsersTestFixture, ImmediateFloat0)
{
    tokenizeAndCreateContext(std::format("{:.1f}", 0.f));
    EXPECT_TRUE(expectParse<ImmediateParser::ImmediateParser>());

    std::shared_ptr<ImmediateOperand> imm;
    EXPECT_TRUE(expectNodeCast<>(imm));

    TEST_FLOAT_IMMEDIATE(0.f);
}

TEST_F(ParsersTestFixture, ImmediateNegativa)
{
    tokenizeAndCreateContext(std::format("{}", -3.141516));
    EXPECT_TRUE(expectParse<ImmediateParser::ImmediateParser>());

    std::shared_ptr<ImmediateOperand> imm;
    EXPECT_TRUE(expectNodeCast(imm));
    EXPECT_TRUE(expectNodeCast(imm));

    TEST_FLOAT_IMMEDIATE(-3.141516);
}

TEST_F(ParsersTestFixture, ImmediateString)
{
    std::string testStr = "\"TEST_STRING\"";
    tokenizeAndCreateContext(testStr);
    EXPECT_TRUE(expectParse<ImmediateParser::ImmediateParser>());

    std::shared_ptr<ImmediateOperand> imm;
    EXPECT_TRUE(expectNodeCast(imm));

    TEST_STRING_IMMEDIATE("TEST_STRING");
}

TEST_F(ParsersTestFixture, ImmediateInvalidInteger)
{
    tokenizeAndCreateContext("0xZZ"); // Invalid hex
    EXPECT_FALSE(expectParse<ImmediateParser::ImmediateParser>());
}

TEST_F(ParsersTestFixture, ImmediateInvalidFloat)
{
    // This might be tokenized as something else or fail parsing
    tokenizeAndCreateContext("3.14.15"); 
    EXPECT_FALSE(expectParse<ImmediateParser::ImmediateParser>());
}

TEST_F(ParsersTestFixture, ImmediateEmptyString)
{
    std::string testStr = "\"\"";
    tokenizeAndCreateContext(testStr);
    EXPECT_TRUE(expectParse<ImmediateParser::ImmediateParser>());

    std::shared_ptr<ImmediateOperand> imm;
    EXPECT_TRUE(expectNodeCast(imm));

    TEST_STRING_IMMEDIATE("");
}
