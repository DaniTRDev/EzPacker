#include "ParsersTestFixture.h"

TEST_F(ParsersTestFixture, InstructionVariableImmediate)
{
    std::string input = "add i64 %variable, 123;";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<InstructionParser::InstructionParser>());
    TEST_INSTRUCTION("add", { AstNodeType::Variable, AstNodeType::Immediate });
}

TEST_F(ParsersTestFixture, InstructionVariableMemory)
{
    std::string input = "lea %variable, i64 (1231);";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<InstructionParser::InstructionParser>());
    TEST_INSTRUCTION("lea", { AstNodeType::Variable, AstNodeType::MemoryOperand });
}

TEST_F(ParsersTestFixture, InstructionImmediateVariable)
{
    std::string input = "instr 123, %myVar;";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<InstructionParser::InstructionParser>());
    TEST_INSTRUCTION("instr", { AstNodeType::Immediate, AstNodeType::Variable });
}

TEST_F(ParsersTestFixture, InstructionImmediateMemory)
{
    std::string input = "instr 123, i64 (%base+0);";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<InstructionParser::InstructionParser>());
    TEST_INSTRUCTION("instr", { AstNodeType::Immediate, AstNodeType::MemoryOperand });
}

TEST_F(ParsersTestFixture, InstructionNoOperands)
{
    std::string input = "nop;";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<InstructionParser::InstructionParser>());
    TEST_INSTRUCTION("nop");
}

TEST_F(ParsersTestFixture, InstructionImmediate)
{
    std::string input = "instr 123;";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<InstructionParser::InstructionParser>());
    TEST_INSTRUCTION("instr", AstNodeType::Immediate);
}

TEST_F(ParsersTestFixture, InstructionVariable)
{
    std::string input = "instr i64 %myVar;";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<InstructionParser::InstructionParser>());
    TEST_INSTRUCTION("instr", AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, InstructionMemory)
{
    std::string input = "jmp i64 (%myVar+0);";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<InstructionParser::InstructionParser>());
    TEST_INSTRUCTION("jmp", AstNodeType::MemoryOperand);
}

TEST_F(ParsersTestFixture, InstructionMissingSemicolon)
{
    std::string input = "nop";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<InstructionParser::InstructionParser>());
}

TEST_F(ParsersTestFixture, InstructionMissingSemicolonWithOperands)
{
    std::string input = "add %a, %b";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<InstructionParser::InstructionParser>());
}

TEST_F(ParsersTestFixture, InstructionInvalidOperand)
{
    std::string input = "add %a, ;";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<InstructionParser::InstructionParser>());
}

TEST_F(ParsersTestFixture, CallInstructionInvalidCallee)
{
    std::string input = "call i64 31();";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<InstructionParser::InstructionParser>());
}

TEST_F(ParsersTestFixture, CallInstructionInvalidReturnType)
{
    std::string input = "call 1231 myFunc();";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<InstructionParser::InstructionParser>());
}

TEST_F(ParsersTestFixture, CallInstructionNoParameters)
{
    std::string input = "call i64 myFunc();";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<InstructionParser::InstructionParser>());
    TEST_CALL_INSTRUCTION("myFunc", "i64");
}

TEST_F(ParsersTestFixture, CallInstruction1Parameter)
{
    std::string input = "call i64 myFunc(%myVar);";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<InstructionParser::InstructionParser>());
    TEST_CALL_INSTRUCTION("myFunc", "i64", AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, CallInstruction2Parameters)
{
    std::string input = "call i64 myFunc(1231, %myVar);";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<InstructionParser::InstructionParser>());
    TEST_CALL_INSTRUCTION("myFunc", "i64", AstNodeType::Immediate, AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, CallInstructionMissingLeftParen)
{
    std::string input = "call i64 myFunc);";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<InstructionParser::InstructionParser>());
}

TEST_F(ParsersTestFixture, CallInstructionMissingRightParen)
{
    std::string input = "call i64 myFunc(;";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<InstructionParser::InstructionParser>());
}

TEST_F(ParsersTestFixture, CallInstructionMissingSemicolon)
{
    std::string input = "call i64 myFunc()";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<InstructionParser::InstructionParser>());
}

TEST_F(ParsersTestFixture, CallInstructionInvalidArg)
{
    std::string input = "call i64 myFunc(invalid)";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<InstructionParser::InstructionParser>());
}
