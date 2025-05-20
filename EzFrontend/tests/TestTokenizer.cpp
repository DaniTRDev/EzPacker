#include <fstream>

#include "tokenizer/BasicTokenizer.h"
#include "IRTypes.h"
#include <gtest/gtest.h>

class BasicTokenizerTest : public ::testing::Test
{
  protected:
    BasicTokenizer tokenizer;
    std::vector<TokenInformation> run(const std::string &input)
    {
        char *buffer = const_cast<char *>(input.c_str());
        EXPECT_TRUE(tokenizer.tokenize(buffer, 0, input.size()));
        return tokenizer.getTokens();
    }
};

TEST_F(BasicTokenizerTest, SingleDot)
{
    auto tokens = run(".");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].m_type, IRTokenType::Dot);
}

TEST_F(BasicTokenizerTest, SingleColon)
{
    auto tokens = run(":");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].m_type, IRTokenType::Colon);
}

TEST_F(BasicTokenizerTest, SingleComma)
{
    auto tokens = run(",");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].m_type, IRTokenType::Comma);
}

TEST_F(BasicTokenizerTest, SingleComment)
{
    auto tokens = run("# this is comment\n");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].m_type, IRTokenType::Comment);
    EXPECT_EQ(tokens[0].m_str, " this is comment");
}

TEST_F(BasicTokenizerTest, SingleIdentifier)
{
    auto tokens = run("My_Var123");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].m_type, IRTokenType::Identifier);
    EXPECT_EQ(tokens[0].m_str, "My_Var123");
}

TEST_F(BasicTokenizerTest, SingleLeftParen)
{
    auto tokens = run("(");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].m_type, IRTokenType::LeftParen);
}

TEST_F(BasicTokenizerTest, SingleRightParen)
{
    auto tokens = run(")");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].m_type, IRTokenType::RightParen);
}

TEST_F(BasicTokenizerTest, NewLine)
{
    auto tokens = run("\n");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].m_type, IRTokenType::NewLine);
}

TEST_F(BasicTokenizerTest, SinglePercentage)
{
    auto tokens = run("%");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].m_type, IRTokenType::Percentage);
}

TEST_F(BasicTokenizerTest, SingleWhiteSpace)
{
    auto tokens = run(" ");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].m_type, IRTokenType::WhiteSpace);
}

TEST_F(BasicTokenizerTest, StringLiteralSimple)
{
    auto tokens = run("\"Hello World\"");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].m_type, IRTokenType::String);
    EXPECT_EQ(tokens[0].m_str, "Hello World");
}

TEST_F(BasicTokenizerTest, StringLiteralWithEscape)
{
    auto tokens = run("\"Line\\nTab\\tQuote\\\"\"");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].m_type, IRTokenType::String);
    EXPECT_EQ(tokens[0].m_str, "Line\nTab\tQuote\"");
}

TEST_F(BasicTokenizerTest, IntegerNumber)
{
    auto tokens = run("12345");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].m_type, IRTokenType::NumberInt);
    EXPECT_EQ(tokens[0].m_str, "12345");
}

TEST_F(BasicTokenizerTest, FloatNumber)
{
    auto tokens = run("3.14159");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].m_type, IRTokenType::NumberFloat);
    EXPECT_EQ(tokens[0].m_str, "3.14159");
}

TEST_F(BasicTokenizerTest, MultipleTokens)
{
    auto tokens = run("add %a, %b\n");
    ASSERT_EQ(tokens.size(), 9);
    EXPECT_EQ(tokens[0].m_type, IRTokenType::Identifier);
    EXPECT_EQ(tokens[1].m_type, IRTokenType::WhiteSpace);
    EXPECT_EQ(tokens[2].m_type, IRTokenType::Percentage);
    EXPECT_EQ(tokens[3].m_type, IRTokenType::Identifier);
    EXPECT_EQ(tokens[4].m_type, IRTokenType::Comma);
    EXPECT_EQ(tokens[5].m_type, IRTokenType::WhiteSpace); // (or possibly NewLine depending on how it's handled)
    EXPECT_EQ(tokens[6].m_type, IRTokenType::Percentage);
    EXPECT_EQ(tokens[7].m_type, IRTokenType::Identifier);
    EXPECT_EQ(tokens[8].m_type, IRTokenType::NewLine);
}

TEST_F(BasicTokenizerTest, InvalidCharacterFails)
{
    char input[] = "@";
    EXPECT_FALSE(tokenizer.tokenize(input, 0, sizeof(input)));
}

TEST_F(BasicTokenizerTest, InvalidFloat)
{
    std::string number = "3.13.2";
    EXPECT_FALSE(tokenizer.tokenize(number.data(), 0, number.size()));
}

TEST_F(BasicTokenizerTest, UnterminatedStringFails)
{
    char input[] = "\"Hello";
    EXPECT_FALSE(tokenizer.tokenize(input, 0, sizeof(input)));
}