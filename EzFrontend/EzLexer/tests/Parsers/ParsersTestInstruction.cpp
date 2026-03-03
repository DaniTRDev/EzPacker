#include "ParsersTestFixture.h"

// =============================================================================
//  Two-operand instructions
// =============================================================================

TEST_F(ParsersTestFixture, Instruction_VarImm)
{
    EXPECT_TRUE(tokenizeAndParse<InstructionParser::InstructionParser>("add i64 %variable, 123;"));
    TEST_INSTRUCTION("add", AstNodeType::Variable, AstNodeType::Immediate);
}

TEST_F(ParsersTestFixture, Instruction_VarMem)
{
    EXPECT_TRUE(tokenizeAndParse<InstructionParser::InstructionParser>("lea %variable, i64 (1231);"));
    TEST_INSTRUCTION("lea", AstNodeType::Variable, AstNodeType::MemoryOperand);
}

TEST_F(ParsersTestFixture, Instruction_ImmVar)
{
    EXPECT_TRUE(tokenizeAndParse<InstructionParser::InstructionParser>("instr 123, %myVar;"));
    TEST_INSTRUCTION("instr", AstNodeType::Immediate, AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, Instruction_ImmMem)
{
    EXPECT_TRUE(tokenizeAndParse<InstructionParser::InstructionParser>("instr 123, i64 (%base+0);"));
    TEST_INSTRUCTION("instr", AstNodeType::Immediate, AstNodeType::MemoryOperand);
}

TEST_F(ParsersTestFixture, Instruction_VarVar)
{
    EXPECT_TRUE(tokenizeAndParse<InstructionParser::InstructionParser>("mov %dst, %src;"));
    TEST_INSTRUCTION("mov", AstNodeType::Variable, AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, Instruction_MemMem)
{
    EXPECT_TRUE(tokenizeAndParse<InstructionParser::InstructionParser>("instr i64 (%a+0), i64 (%b+0);"));
    TEST_INSTRUCTION("instr", AstNodeType::MemoryOperand, AstNodeType::MemoryOperand);
}

// =============================================================================
//  Zero-operand instructions
// =============================================================================

TEST_F(ParsersTestFixture, Instruction_Nop)
{
    EXPECT_TRUE(tokenizeAndParse<InstructionParser::InstructionParser>("nop;"));
    TEST_INSTRUCTION("nop");
}

TEST_F(ParsersTestFixture, Instruction_Halt)
{
    EXPECT_TRUE(tokenizeAndParse<InstructionParser::InstructionParser>("halt;"));
    TEST_INSTRUCTION("halt");
}

// =============================================================================
//  One-operand instructions
// =============================================================================

TEST_F(ParsersTestFixture, Instruction_SingleImm)
{
    EXPECT_TRUE(tokenizeAndParse<InstructionParser::InstructionParser>("instr 123;"));
    TEST_INSTRUCTION("instr", AstNodeType::Immediate);
}

TEST_F(ParsersTestFixture, Instruction_SingleVar)
{
    EXPECT_TRUE(tokenizeAndParse<InstructionParser::InstructionParser>("instr i64 %myVar;"));
    TEST_INSTRUCTION("instr", AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, Instruction_SingleMem)
{
    EXPECT_TRUE(tokenizeAndParse<InstructionParser::InstructionParser>("jmp i64 (%myVar+0);"));
    TEST_INSTRUCTION("jmp", AstNodeType::MemoryOperand);
}

// =============================================================================
//  Instruction error cases
// =============================================================================

TEST_F(ParsersTestFixture, Instruction_MissingSemicolon)
{
    EXPECT_FALSE(tokenizeAndParse<InstructionParser::InstructionParser>("nop"));
}

TEST_F(ParsersTestFixture, Instruction_MissingSemicolonWithOperands)
{
    EXPECT_FALSE(tokenizeAndParse<InstructionParser::InstructionParser>("add %a, %b"));
}

TEST_F(ParsersTestFixture, Instruction_TrailingCommaBeforeSemicolon)
{
    EXPECT_FALSE(tokenizeAndParse<InstructionParser::InstructionParser>("add %a, ;"));
}

// =============================================================================
//  Call instruction – valid cases
// =============================================================================

TEST_F(ParsersTestFixture, CallInstruction_NoParams)
{
    EXPECT_TRUE(tokenizeAndParse<InstructionParser::InstructionParser>("call i64 myFunc();"));
    TEST_CALL_INSTRUCTION("myFunc", "i64");
}

TEST_F(ParsersTestFixture, CallInstruction_1Param)
{
    EXPECT_TRUE(tokenizeAndParse<InstructionParser::InstructionParser>("call i64 myFunc(%myVar);"));
    TEST_CALL_INSTRUCTION("myFunc", "i64", AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, CallInstruction_2Params)
{
    EXPECT_TRUE(tokenizeAndParse<InstructionParser::InstructionParser>("call i64 myFunc(1231, %myVar);"));
    TEST_CALL_INSTRUCTION("myFunc", "i64", AstNodeType::Immediate, AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, CallInstruction_3Params)
{
    EXPECT_TRUE(tokenizeAndParse<InstructionParser::InstructionParser>("call void myFunc(%a, %b, %c);"));
    TEST_CALL_INSTRUCTION("myFunc", "void", AstNodeType::Variable, AstNodeType::Variable, AstNodeType::Variable);
}

TEST_F(ParsersTestFixture, CallInstruction_VoidReturn)
{
    EXPECT_TRUE(tokenizeAndParse<InstructionParser::InstructionParser>("call void doNothing();"));
    TEST_CALL_INSTRUCTION("doNothing", "void");
}

// =============================================================================
//  Call instruction – error cases
// =============================================================================

TEST_F(ParsersTestFixture, CallInstruction_InvalidCallee)
{
    EXPECT_FALSE(tokenizeAndParse<InstructionParser::InstructionParser>("call i64 31();"));
}

TEST_F(ParsersTestFixture, CallInstruction_InvalidReturnType)
{
    EXPECT_FALSE(tokenizeAndParse<InstructionParser::InstructionParser>("call 1231 myFunc();"));
}

TEST_F(ParsersTestFixture, CallInstruction_MissingLeftParen)
{
    EXPECT_FALSE(tokenizeAndParse<InstructionParser::InstructionParser>("call i64 myFunc);"));
}

TEST_F(ParsersTestFixture, CallInstruction_MissingRightParen)
{
    EXPECT_FALSE(tokenizeAndParse<InstructionParser::InstructionParser>("call i64 myFunc(;"));
}

TEST_F(ParsersTestFixture, CallInstruction_MissingSemicolon)
{
    EXPECT_FALSE(tokenizeAndParse<InstructionParser::InstructionParser>("call i64 myFunc()"));
}

TEST_F(ParsersTestFixture, CallInstruction_InvalidArg)
{
    EXPECT_FALSE(tokenizeAndParse<InstructionParser::InstructionParser>("call i64 myFunc(invalid)"));
}
