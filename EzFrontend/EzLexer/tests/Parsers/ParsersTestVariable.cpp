#include "ParsersTestFixture.h"

TEST_F(ParsersTestFixture, GlobalVariableSingleInitializer)
{
    std::string input = R"(i8 %myVar: 12345)";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<VariableParser>());
    TEST_VARIABLE(false, "i8", "myVar", AstNodeType::Immediate);
}

TEST_F(ParsersTestFixture, GlobalVariableInvalidSingleInitializer)
{
    std::string input = R"(i8 %myVar: )";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<VariableParser>());
}

TEST_F(ParsersTestFixture, GlobalVariableArray)
{
    std::string input = R"(i8 %myVar: {1234, 1231, 0xFFFFFF})";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<VariableParser>());
    TEST_VARIABLE(true, "i8", "myVar", AstNodeType::Immediate, AstNodeType::Immediate, AstNodeType::Immediate);
}

TEST_F(ParsersTestFixture, GlobalVariableInvalidArray)
{
    std::string input = R"(i8 %myVar: {})";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<VariableParser>());
}

TEST_F(ParsersTestFixture, LocalVariable)
{
    std::string input = R"(%myVar)";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<VariableParser>());
    TEST_VARIABLE(false, "", "myVar", );
}
