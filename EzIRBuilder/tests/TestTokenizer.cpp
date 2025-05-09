#include <fstream>

#include "parser/tokenizer/BasicTokenizer.h"
#include <gtest/gtest.h>

class TokenizerTest : public ::testing::Test
{
  protected:
    size_t m_currIndex;
    BasicTokenizer tokenizer;

    void SetUp() override
    {
        m_currIndex = 0;
        tokenizer.addInstructionTable(g_instructionTable);
        tokenizer.addKeywordMap(g_keywordTokenMap);
        tokenizer.addTypeMap(g_typeTokenMap);
    }

    IRTokenType getType(size_t index = 0) const
    {
        return tokenizer.getTokens()[index].m_type;
    }

    std::string getTokenStr(size_t index = 0) const
    {
        return tokenizer.getTokens()[index].m_str;
    }

    size_t tokenizeInput(const std::string &input)
    {
        std::vector<char> buffer(input.begin(), input.end());
        return tokenizer.tokenize(buffer.data(), 0, buffer.size());
    }

    bool checkIdentifierToken(std::string content)
    {
        auto &tokens = tokenizer.getTokens();
        return m_currIndex < tokens.size() && tokens[m_currIndex].m_type == IRTokenType::Identifier &&
               tokens[m_currIndex++].m_str == content;
    }

    bool checkInstructionToken(std::string content)
    {
        auto &tokens = tokenizer.getTokens();
        return m_currIndex < tokens.size() && tokens[m_currIndex].m_type == IRTokenType::Instruction &&
               tokens[m_currIndex++].m_str == content;
    }

    bool checkLabel(std::string labelName)
    {
        return checkTypeAndContent(IRTokenType::Label, ".label") && checkIdentifierToken(labelName) &&
               checkTypeAndContent(IRTokenType::DoubleDot, ":");
    }

    bool checkNumberIntToken(std::string content)
    {
        auto &tokens = tokenizer.getTokens();
        return m_currIndex < tokens.size() && tokens[m_currIndex].m_type == IRTokenType::NumberInt &&
               tokens[m_currIndex++].m_str == content;
    }

    bool checkNumberFloatToken(std::string content)
    {
        auto &tokens = tokenizer.getTokens();
        return m_currIndex < tokens.size() && tokens[m_currIndex].m_type == IRTokenType::NumberFloat &&
               tokens[m_currIndex++].m_str == content;
    }

    bool checkRegisterToken(std::string content)
    {
        auto &tokens = tokenizer.getTokens();
        return m_currIndex < tokens.size() && tokens[m_currIndex].m_type == IRTokenType::Register &&
               tokens[m_currIndex++].m_str == content;
    }

    bool checkStringToken(std::string content)
    {
        auto &tokens = tokenizer.getTokens();
        return m_currIndex < tokens.size() && tokens[m_currIndex].m_type == IRTokenType::String &&
               tokens[m_currIndex++].m_str == content;
    }

    bool checkTypeToken(IRTokenType type)
    {
        auto &tokens = tokenizer.getTokens();
        return m_currIndex < tokens.size() && tokens[m_currIndex++].m_type == type;
    }

    bool checkTypeAndContent(IRTokenType type, std::string content)
    {
        auto &tokens = tokenizer.getTokens();
        return m_currIndex < tokens.size() && tokens[m_currIndex].m_type == type &&
               content == tokens[m_currIndex++].m_str;
    }
};

TEST_F(TokenizerTest, TokenizeInvalidKeyword)
{
    std::string input = ".mov";
    EXPECT_FALSE(tokenizeInput(input));
}

TEST_F(TokenizerTest, TokenizeRegister)
{
    std::string input = "%eax";
    EXPECT_TRUE(tokenizeInput(input));
    EXPECT_TRUE(checkRegisterToken("%eax"));
}

TEST_F(TokenizerTest, TokenizeIdentifier)
{
    std::string input = "my_var";
    EXPECT_TRUE(tokenizeInput(input));
    EXPECT_TRUE(checkIdentifierToken("my_var"));
}

TEST_F(TokenizerTest, TokenizeInteger)
{
    std::string input = "12345";
    EXPECT_TRUE(tokenizeInput(input));
    EXPECT_TRUE(checkNumberIntToken("12345"));
}

TEST_F(TokenizerTest, TokenizeFloat)
{
    std::string input = "3.14";
    EXPECT_TRUE(tokenizeInput(input));
    EXPECT_TRUE(checkNumberFloatToken("3.14"));
}

TEST_F(TokenizerTest, TokenizeMemory)
{
    std::string input = "(%eax %eax %eax %eax)";
    // EXPECT_TRUE(tokenizeInput(input));
    // EXPECT_EQ(getType(), IRTokenType::NumberFloat);
}

TEST_F(TokenizerTest, TokenizeInvalidDoubleDot)
{
    std::string input = "3.1.4";
    EXPECT_FALSE(tokenizeInput(input));
}

TEST_F(TokenizerTest, TokenizeMultipleTokens)
{
    std::string input = ".store %eax 42";
    EXPECT_TRUE(tokenizeInput(input));
    EXPECT_TRUE(checkInstructionToken(".store"));
    EXPECT_TRUE(checkRegisterToken("%eax"));
    EXPECT_TRUE(checkNumberIntToken("42"));
}

TEST_F(TokenizerTest, TokenizeMultipleWhiteSpaces)
{
    std::string input = ".i8        .i16       .i32      .i64      %spaced      _ASD_1";
    EXPECT_TRUE(tokenizeInput(input));
    EXPECT_TRUE(checkTypeToken(IRTokenType::TypeI8));
    EXPECT_TRUE(checkTypeToken(IRTokenType::TypeI16));
    EXPECT_TRUE(checkTypeToken(IRTokenType::TypeI32));
    EXPECT_TRUE(checkTypeToken(IRTokenType::TypeI64));
    EXPECT_TRUE(checkRegisterToken("%spaced"));
    EXPECT_TRUE(checkIdentifierToken("_ASD_1"));
}

TEST_F(TokenizerTest, TestBigTokenList)
{
    std::string input = ".module %r1 100 varA 3.14\n"
                        ".add %r2 200 varB 2.718\n"
                        ".sub %r3 300 varC 0.577\n"
                        ".load %r4 400 varD 1.618\n"
                        ".store %r5 500 varE 6.022\n"
                        ".i64 .i64 .i64 .ptr .i16 .ptr";

    EXPECT_TRUE(tokenizeInput(input));

    std::vector<std::pair<IRTokenType, std::string>> expected = {
        {IRTokenType::Module, ".module"},    {IRTokenType::Register, "%r1"},      {IRTokenType::NumberInt, "100"},
        {IRTokenType::Identifier, "varA"},   {IRTokenType::NumberFloat, "3.14"},  {IRTokenType::Instruction, ".add"},
        {IRTokenType::Register, "%r2"},      {IRTokenType::NumberInt, "200"},     {IRTokenType::Identifier, "varB"},
        {IRTokenType::NumberFloat, "2.718"}, {IRTokenType::Instruction, ".sub"},  {IRTokenType::Register, "%r3"},
        {IRTokenType::NumberInt, "300"},     {IRTokenType::Identifier, "varC"},   {IRTokenType::NumberFloat, "0.577"},
        {IRTokenType::Instruction, ".load"}, {IRTokenType::Register, "%r4"},      {IRTokenType::NumberInt, "400"},
        {IRTokenType::Identifier, "varD"},   {IRTokenType::NumberFloat, "1.618"}, {IRTokenType::Instruction, ".store"},
        {IRTokenType::Register, "%r5"},      {IRTokenType::NumberInt, "500"},     {IRTokenType::Identifier, "varE"},
        {IRTokenType::NumberFloat, "6.022"}, {IRTokenType::TypeI64, ".i64"},      {IRTokenType::TypeI64, ".i64"},
        {IRTokenType::TypeI64, ".i64"},      {IRTokenType::TypePtr, ".ptr"},      {IRTokenType::TypeI16, ".i16"},
        {IRTokenType::TypePtr, ".ptr"}};

    ASSERT_EQ(tokenizer.getTokens().size(), expected.size());

    for (size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_EQ(tokenizer.getTokens()[i].m_type, expected[i].first);
        EXPECT_EQ(tokenizer.getTokens()[i].m_str, expected[i].second);
    }
}

TEST_F(TokenizerTest, TokenizeBigList2)
{
    std::string input = ".load %eax 123 45.67 var1 var_2 .store %ebx 0 .add %ecx\n"
                        "45678\n"
                        ".sub %edx";

    EXPECT_TRUE(tokenizeInput(input));
    std::vector<std::pair<IRTokenType, std::string>> expected = {
        {IRTokenType::Instruction, ".load"},  {IRTokenType::Register, "%eax"},   {IRTokenType::NumberInt, "123"},
        {IRTokenType::NumberFloat, "45.67"},  {IRTokenType::Identifier, "var1"}, {IRTokenType::Identifier, "var_2"},
        {IRTokenType::Instruction, ".store"}, {IRTokenType::Register, "%ebx"},   {IRTokenType::NumberInt, "0"},
        {IRTokenType::Instruction, ".add"},   {IRTokenType::Register, "%ecx"},   {IRTokenType::NumberInt, "45678"},
        {IRTokenType::Instruction, ".sub"},   {IRTokenType::Register, "%edx"},
    };

    ASSERT_EQ(tokenizer.getTokens().size(), expected.size());

    for (size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_EQ(tokenizer.getTokens()[i].m_type, expected[i].first);
        EXPECT_EQ(tokenizer.getTokens()[i].m_str, expected[i].second);
    }
}

TEST_F(TokenizerTest, TokenizeSimpleString)
{
    std::string input = "\"Simple Test String\"";
    EXPECT_TRUE(tokenizeInput(input));
    EXPECT_TRUE(checkStringToken("Simple Test String"));
}

TEST_F(TokenizerTest, TokenizeBackslashString)
{
    std::string input = "\"\\\\ \\\\ \\n \\r\"";
    EXPECT_TRUE(tokenizeInput(input));
    EXPECT_TRUE(checkStringToken("\\ \\ \n \r"));
}

TEST_F(TokenizerTest, TokenizeStringAndInstruction)
{
    std::string input = "\"\\\\ \\\\ \\n \\r .add\" .sub";
    EXPECT_TRUE(tokenizeInput(input));
    EXPECT_TRUE(checkStringToken("\\ \\ \n \r .add"));
    EXPECT_TRUE(checkInstructionToken(".sub"));
}

TEST_F(TokenizerTest, TokenizeMultiLineString)
{
    std::string input =
        "\"-Part1-This is a multiline\"                              \\\"-Part2-This is a multiline\".add %eax";
    // Formated string: -Part1-This is a multiline-Part2-This is a multiline

    EXPECT_TRUE(tokenizeInput(input));
    EXPECT_EQ(tokenizer.getTokens().size(), 3);
    EXPECT_TRUE(checkStringToken("-Part1-This is a multiline-Part2-This is a multiline"));
    EXPECT_TRUE(checkInstructionToken(".add"));
    EXPECT_TRUE(checkRegisterToken("%eax"));
}

TEST_F(TokenizerTest, TokenizeWithInvalidCharacter)
{
    std::string input = "abc$def";
    EXPECT_FALSE(tokenizeInput(input));
}

TEST_F(TokenizerTest, TokenizeEmptyBuffer)
{
    EXPECT_FALSE(tokenizeInput(""));
}

TEST_F(TokenizerTest, TokenizeNullBuffer)
{
    EXPECT_FALSE(tokenizer.tokenize(nullptr, 0, 0));
}

TEST_F(TokenizerTest, TokenizeTestFile)
{
    std::ifstream input("testData/TestCompleteCode.txt");
    EXPECT_TRUE(input.is_open());

    std::string content;
    while (!input.eof())
    {
        std::string line;
        std::getline(input, line);

        content += line + '\n';
    }

    EXPECT_FALSE(content.empty());
    EXPECT_TRUE(tokenizer.tokenize(content.data(), 0, content.size()));

    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Arch, ".arch"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Identifier, "x86_64"));

    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Comment, "# Global variable declarations"));

    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Variable, ".variable"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Identifier, "globalCounter"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::DoubleDot, ":"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::TypeI32, ".i32"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "0"));

    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Variable, ".variable"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Identifier, "matrix"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::DoubleDot, ":"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Vector, ".vector"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::TypeI16, ".i16"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "9"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "1"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "2"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "3"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "4"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "5"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "6"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "7"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "8"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "9"));

    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Variable, ".variable"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Identifier, "ptrArray"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::DoubleDot, ":"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Vector, ".vector"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::TypePtr, ".ptr"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::TypeI8, ".i8"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "3"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "1000"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "2000"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "3000"));

    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Variable, ".variable"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Identifier, "tempVec"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::DoubleDot, ":"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Vector, ".vector"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::TypeI32, ".i32"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "5"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "10"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "20"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "30"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "40"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "50"));

    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Comment, "# Module using different data types"));

    // Check module declaration
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Module, ".module"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Identifier, "TypeTest"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::DoubleDot, ":"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Identifier, "myArg"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::TypeI8, ".i8"));

    // Check .reserveStack, .push, and .label
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Instruction, ".reserveStack"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::TypeI8, ".i8"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "32"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Instruction, ".push"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Register, "%stackFrame"));
    EXPECT_TRUE(checkLabel("myLabel1"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Instruction, ".load"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Register, "%reg_i8"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Comma, ","));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "255"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Instruction, ".load"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Register, "%reg_i16"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Comma, ","));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "65535"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Instruction, ".load"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Register, "%reg_i32"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Comma, ","));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "4294967295"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Instruction, ".load"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Register, "%reg_i64"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Comma, ","));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "9223372036854775807"));

    // Check second label and store instructions
    EXPECT_TRUE(checkLabel("myLabel2"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Instruction, ".store"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::TypePtr, ".ptr"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::TypeI8, ".i8"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::LeftParen, "("));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Register, "%stackFrame"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Comma, ","));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "0"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::RightParen, ")"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Comma, ","));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Register, "%reg_i8"));

    // Check third label and load instructions
    EXPECT_TRUE(checkLabel("myLabel3"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Instruction, ".load"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Register, "%tmp1"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Comma, ","));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::TypePtr, ".ptr"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::TypeI8, ".i8"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::LeftParen, "("));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Register, "%stackFrame"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Comma, ","));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "0"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::RightParen, ")"));

    // Check pop, freeStack, and ret
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Instruction, ".pop"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Register, "%stackFrame"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Instruction, ".freeStack"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::TypeI8, ".i8"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::NumberInt, "32"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::Instruction, ".ret"));
    EXPECT_TRUE(checkTypeAndContent(IRTokenType::End, ".end"));
}