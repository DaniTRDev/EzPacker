#include "TokenizerTestFixture.h"

TEST_F(TokenizerTestFixture, TestSingleThread_UnknownSymbol)
{
    auto tokenizer = createBasicTokenizer();
    std::string buffer = "@";

    EXPECT_FALSE(expectTokenizeResult(buffer));
}

TEST_F(TokenizerTestFixture, TestSingleThread_IdentifierSimple)
{
    auto tokenizer = createBasicTokenizer();
    std::string buffer = "myIdentifier";

    EXPECT_TRUE(expectTokenizeResult(buffer));
    EXPECT_TRUE(expectTokenCount(1));
    EXPECT_TRUE(expectTokenType(_TokenType::Identifier));
    EXPECT_TRUE(expectTokenContent("myIdentifier"));
    EXPECT_FALSE(advanceToken());
}

TEST_F(TokenizerTestFixture, TestSingleThread_IdentifierWithNumbers)
{
    auto tokenizer = createBasicTokenizer();
    std::string buffer = "myIdentifier2345";

    EXPECT_TRUE(expectTokenizeResult(buffer));
    EXPECT_TRUE(expectTokenCount(1));
    EXPECT_TRUE(expectTokenType(_TokenType::Identifier));
    EXPECT_TRUE(expectTokenContent("myIdentifier2345"));
    EXPECT_FALSE(advanceToken());
}

TEST_F(TokenizerTestFixture, TestSingleThread_IdentifierWithNumbersAndUnderscores)
{
    auto tokenizer = createBasicTokenizer();
    std::string buffer = "_myIdentifier_2345";

    EXPECT_TRUE(expectTokenizeResult(buffer));
    EXPECT_TRUE(expectTokenCount(1));
    EXPECT_TRUE(expectTokenType(_TokenType::Identifier));
    EXPECT_TRUE(expectTokenContent("_myIdentifier_2345"));
    EXPECT_FALSE(advanceToken());
}

TEST_F(TokenizerTestFixture, TestSingleThread_NumberIntDecimal)
{
    auto tokenizer = createBasicTokenizer();
    std::string buffer = "1231312";

    EXPECT_TRUE(expectTokenizeResult(buffer));
    EXPECT_TRUE(expectTokenCount(1));
    EXPECT_TRUE(expectTokenType(_TokenType::NumberInt));
    EXPECT_TRUE(expectTokenContent("1231312"));
    EXPECT_FALSE(advanceToken());
}

TEST_F(TokenizerTestFixture, TestSingleThread_NumberNegativeIntDecimal)
{
    auto tokenizer = createBasicTokenizer();
    std::string buffer = "-1231312";

    EXPECT_TRUE(expectTokenizeResult(buffer));
    EXPECT_TRUE(expectTokenCount(2));
    EXPECT_TRUE(expectTokenType(_TokenType::Minus));
    EXPECT_TRUE(advanceToken());
    EXPECT_TRUE(expectTokenType(_TokenType::NumberInt));
}

TEST_F(TokenizerTestFixture, TestSingleThread_NumberIntHex)
{
    auto tokenizer = createBasicTokenizer();
    std::string buffer = "0x1231312";

    EXPECT_TRUE(expectTokenizeResult(buffer));
    EXPECT_TRUE(expectTokenCount(1));
    EXPECT_TRUE(expectTokenType(_TokenType::NumberInt));
    EXPECT_TRUE(expectTokenContent("0x1231312"));
    EXPECT_FALSE(advanceToken());
}

TEST_F(TokenizerTestFixture, TestSingleThread_NumberNegativeIntHex)
{
    auto tokenizer = createBasicTokenizer();
    std::string buffer = "-0x1231312";

    EXPECT_TRUE(expectTokenizeResult(buffer));
    EXPECT_TRUE(expectTokenCount(2));
    EXPECT_TRUE(expectTokenType(_TokenType::Minus));
    EXPECT_TRUE(advanceToken());
    EXPECT_TRUE(expectTokenType(_TokenType::NumberInt));
}

TEST_F(TokenizerTestFixture, TestSingleThread_NumberIntHexNoPrefix)
{
    auto tokenizer = createBasicTokenizer();
    std::string buffer = "FFAB";

    EXPECT_TRUE(expectTokenizeResult(buffer));
    EXPECT_TRUE(expectTokenType(_TokenType::Identifier));
    EXPECT_TRUE(expectTokenContent("FFAB"));
}

TEST_F(TokenizerTestFixture, TestSingleThread_NumberIntHexInvalidPrefix)
{
    auto tokenizer = createBasicTokenizer();
    std::string buffer = "xFFAB";

    EXPECT_TRUE(expectTokenizeResult(buffer));
    EXPECT_TRUE(expectTokenType(_TokenType::Identifier));
    EXPECT_TRUE(expectTokenContent("xFFAB"));
}

TEST_F(TokenizerTestFixture, TestSingleThread_NumberFloat)
{
    auto tokenizer = createBasicTokenizer();
    std::string buffer = "12.31312";

    EXPECT_TRUE(expectTokenizeResult(buffer));
    EXPECT_TRUE(expectTokenCount(1));
    EXPECT_TRUE(expectTokenType(_TokenType::NumberFloat));
    EXPECT_TRUE(expectTokenContent("12.31312"));
    EXPECT_FALSE(advanceToken());
}

TEST_F(TokenizerTestFixture, TestSingleThread_NumberFloatInvalidDots)
{
    auto tokenizer = createBasicTokenizer();
    std::string buffer = "12.313.12";

    EXPECT_FALSE(expectTokenizeResult(buffer));
}

TEST_F(TokenizerTestFixture, TestSingleThread_StringSimple)
{
    auto tokenizer = createBasicTokenizer();
    std::string buffer = "\"My Test String\"";

    EXPECT_TRUE(expectTokenizeResult(buffer));
    EXPECT_TRUE(expectTokenCount(1));
    EXPECT_TRUE(expectTokenType(_TokenType::String));
    EXPECT_TRUE(expectTokenContent("My Test String"));
    EXPECT_FALSE(advanceToken());
}

TEST_F(TokenizerTestFixture, TestSingleThread_StringWithEscaping)
{
    auto tokenizer = createBasicTokenizer();
    std::string buffer = "\"My Test \\n String \\r\"";

    EXPECT_TRUE(expectTokenizeResult(buffer));
    EXPECT_TRUE(expectTokenCount(1));
    EXPECT_TRUE(expectTokenType(_TokenType::String));
    EXPECT_TRUE(expectTokenContent("My Test \n String \r"));
    EXPECT_FALSE(advanceToken());
}

TEST_F(TokenizerTestFixture, TestSingleThread_StringWithInvalidEscaping)
{
    auto tokenizer = createBasicTokenizer();
    std::string buffer = "\"My Test \\{ String\"";

    EXPECT_FALSE(expectTokenizeResult(buffer));
}

TEST_F(TokenizerTestFixture, TestSingleThread_Comments)
{
    auto tokenizer = createBasicTokenizer();
    std::string buffer = "# This is a comment";

    EXPECT_TRUE(expectTokenizeResult(buffer));
    EXPECT_TRUE(expectTokenCount(0));
}

TEST_F(TokenizerTestFixture, TestSingleThread_SingleTokens)
{
    auto tokenizer = createBasicTokenizer();
    std::string buffer = "{} () % : ; . ,"; // White spaces are ignored.

    EXPECT_TRUE(expectTokenizeResult(buffer));
    EXPECT_TRUE(expectTokenCount(9));

    EXPECT_TRUE(expectTokenType(_TokenType::LeftBrace));
    EXPECT_TRUE(advanceToken());
    EXPECT_TRUE(expectTokenType(_TokenType::RightBrace));
    EXPECT_TRUE(advanceToken());
    EXPECT_TRUE(expectTokenType(_TokenType::LeftParen));
    EXPECT_TRUE(advanceToken());
    EXPECT_TRUE(expectTokenType(_TokenType::RightParen));
    EXPECT_TRUE(advanceToken());
    EXPECT_TRUE(expectTokenType(_TokenType::Percentage));
    EXPECT_TRUE(advanceToken());
    EXPECT_TRUE(expectTokenType(_TokenType::Colon));
    EXPECT_TRUE(advanceToken());
    EXPECT_TRUE(expectTokenType(_TokenType::SemiColon));
    EXPECT_TRUE(advanceToken());
    EXPECT_TRUE(expectTokenType(_TokenType::Dot));
    EXPECT_TRUE(advanceToken());
    EXPECT_TRUE(expectTokenType(_TokenType::Comma));
    EXPECT_FALSE(advanceToken());
}