#include "BasicParserTest.h"

TEST(TestInstructionParser, TestValidInstruction)
{
    auto nodes = BasicParserTest::testRule(false, NodeParsers::Instruction(), ".add .i64 %rcx, .i32 (, %rcx, 4);");
    BasicParserTest::expectChildCount(1, nodes);

    auto instr = nodes->getChild(0);
    BasicParserTest::expectChildCount(3, instr);
    BasicParserTest::expectChildNodeType(AstNodes::Identifier().getId(), 0, instr); // "add"
    BasicParserTest::expectChildNodeType(AstNodes::InstructionOperand().getId(), 1, instr);
    BasicParserTest::expectChildNodeType(AstNodes::InstructionOperand().getId(), 2, instr);

    auto operand1 = instr->getChild(1);
    EXPECT_NE(operand1, nullptr);
    BasicParserTest::expectChildCount(2, operand1);
    BasicParserTest::expectChildNodeType(AstNodes::Type().getId(), 0, operand1);
    BasicParserTest::expectChildNodeType(AstNodes::VirtualVariable().getId(), 1, operand1);

    auto operand2 = instr->getChild(2);
    EXPECT_NE(operand2, nullptr);
    BasicParserTest::expectChildCount(2, operand2);
    BasicParserTest::expectChildNodeType(AstNodes::Type().getId(), 0, operand2);
    BasicParserTest::expectChildNodeType(AstNodes::IndexScaleMemory().getId(), 1, operand2);
}

TEST(TestInstructionParser, TestValidInstruction2)
{
    auto nodes = BasicParserTest::testRule(false, NodeParsers::Instruction(), ".add .i64 %rcx, .i64 %rcx;");
    BasicParserTest::expectChildCount(1, nodes);

    auto instr = nodes->getChild(0);
    BasicParserTest::expectChildCount(3, instr);
    BasicParserTest::expectChildNodeType(AstNodes::Identifier().getId(), 0, instr); // "add"
    BasicParserTest::expectChildNodeType(AstNodes::InstructionOperand().getId(), 1, instr);
    BasicParserTest::expectChildNodeType(AstNodes::InstructionOperand().getId(), 2, instr);

    auto operand1 = instr->getChild(1);
    EXPECT_NE(operand1, nullptr);
    BasicParserTest::expectChildCount(2, operand1);
    BasicParserTest::expectChildNodeType(AstNodes::Type().getId(), 0, operand1);
    BasicParserTest::expectChildNodeType(AstNodes::VirtualVariable().getId(), 1, operand1);

    auto operand2 = instr->getChild(2);
    EXPECT_NE(operand2, nullptr);
    BasicParserTest::expectChildCount(2, operand2);
    BasicParserTest::expectChildNodeType(AstNodes::Type().getId(), 0, operand2);
    BasicParserTest::expectChildNodeType(AstNodes::VirtualVariable().getId(), 1, operand2);
}

TEST(TestInstructionParser, TestValidInstruction3)
{
    auto nodes = BasicParserTest::testRule(false, NodeParsers::Instruction(), ".add .i64 %rcx, .i64 123423;");
    BasicParserTest::expectChildCount(1, nodes);

    auto instr = nodes->getChild(0);
    BasicParserTest::expectChildCount(3, instr);
    BasicParserTest::expectChildNodeType(AstNodes::Identifier().getId(), 0, instr); // "add"
    BasicParserTest::expectChildNodeType(AstNodes::InstructionOperand().getId(), 1, instr);
    BasicParserTest::expectChildNodeType(AstNodes::InstructionOperand().getId(), 2, instr);

    auto operand1 = instr->getChild(1);
    EXPECT_NE(operand1, nullptr);
    BasicParserTest::expectChildCount(2, operand1);
    BasicParserTest::expectChildNodeType(AstNodes::Type().getId(), 0, operand1);
    BasicParserTest::expectChildNodeType(AstNodes::VirtualVariable().getId(), 1, operand1);

    auto operand2 = instr->getChild(2);
    EXPECT_NE(operand2, nullptr);
    BasicParserTest::expectChildCount(2, operand2);
    BasicParserTest::expectChildNodeType(AstNodes::Type().getId(), 0, operand2);
    BasicParserTest::expectChildNodeType(AstNodes::IntNumber().getId(), 1, operand2);
}

TEST(TestInstructionParser, TestValidInstruction4)
{
    auto nodes = BasicParserTest::testRule(false, NodeParsers::Instruction(), ".add .i32 (, %rcx, 3), .i64 %rcx;");
    BasicParserTest::expectChildCount(1, nodes);

    auto instr = nodes->getChild(0);
    BasicParserTest::expectChildCount(3, instr);
    BasicParserTest::expectChildNodeType(AstNodes::Identifier().getId(), 0, instr); // "add"
    BasicParserTest::expectChildNodeType(AstNodes::InstructionOperand().getId(), 1, instr);
    BasicParserTest::expectChildNodeType(AstNodes::InstructionOperand().getId(), 2, instr);

    auto operand1 = instr->getChild(1);
    EXPECT_NE(operand1, nullptr);
    BasicParserTest::expectChildCount(2, operand1);
    BasicParserTest::expectChildNodeType(AstNodes::Type().getId(), 0, operand1);
    BasicParserTest::expectChildNodeType(AstNodes::IndexScaleMemory().getId(), 1, operand1);

    auto operand2 = instr->getChild(2);
    EXPECT_NE(operand1, nullptr);
    BasicParserTest::expectChildCount(2, operand2);
    BasicParserTest::expectChildNodeType(AstNodes::Type().getId(), 0, operand2);
    BasicParserTest::expectChildNodeType(AstNodes::VirtualVariable().getId(), 1, operand2);
}

TEST(TestInstructionParser, TestValidInstruction5)
{
    auto nodes = BasicParserTest::testRule(false, NodeParsers::Instruction(), ".add .i32 (%rax, 4), .i32 3452432;");
    BasicParserTest::expectChildCount(1, nodes);

    auto instr = nodes->getChild(0);
    BasicParserTest::expectChildCount(3, instr);
    BasicParserTest::expectChildNodeType(AstNodes::Identifier().getId(), 0, instr); // "add"
    BasicParserTest::expectChildNodeType(AstNodes::InstructionOperand().getId(), 1, instr);
    BasicParserTest::expectChildNodeType(AstNodes::InstructionOperand().getId(), 2, instr);

    auto operand1 = instr->getChild(1);
    EXPECT_NE(operand1, nullptr);
    BasicParserTest::expectChildCount(2, operand1);
    BasicParserTest::expectChildNodeType(AstNodes::Type().getId(), 0, operand1);
    BasicParserTest::expectChildNodeType(AstNodes::BaseDisplMemory().getId(), 1, operand1);

    auto operand2 = instr->getChild(2);
    EXPECT_NE(operand1, nullptr);
    BasicParserTest::expectChildCount(2, operand2);
    BasicParserTest::expectChildNodeType(AstNodes::Type().getId(), 0, operand2);
    BasicParserTest::expectChildNodeType(AstNodes::IntNumber().getId(), 1, operand2);
}

TEST(TestInstructionParser, TestValidInstructionSingleOperand)
{
    auto nodes = BasicParserTest::testRule(false, NodeParsers::Instruction(), ".push .i32 (%rax, 4);");
    BasicParserTest::expectChildCount(1, nodes);

    auto instr = nodes->getChild(0);
    BasicParserTest::expectChildCount(2, instr);
    BasicParserTest::expectChildNodeType(AstNodes::Identifier().getId(), 0, instr); // "add"
    BasicParserTest::expectChildNodeType(AstNodes::InstructionOperand().getId(), 1, instr);

    auto operand1 = instr->getChild(1);
    EXPECT_NE(operand1, nullptr);
    BasicParserTest::expectChildCount(2, operand1);
    BasicParserTest::expectChildNodeType(AstNodes::Type().getId(), 0, operand1);
    BasicParserTest::expectChildNodeType(AstNodes::BaseDisplMemory().getId(), 1, operand1);
}

TEST(TestInstructionParser, TestInvalidInstructionName)
{
    auto nodes = BasicParserTest::testRule(true, NodeParsers::Instruction(), "add .i64 (%rax, 4), .i32 3452432;");
    BasicParserTest::expectChildCount(0, nodes);
}

TEST(TestInstructionParser, TestInvalidInstructionType)
{
    auto nodes = BasicParserTest::testRule(true, NodeParsers::Instruction(), ".add i64 (%rax, 4), .i32 3452432;");
    BasicParserTest::expectChildCount(0, nodes);
}

TEST(TestInstructionParser, TestInvalidInstructionOperand1)
{
    auto nodes = BasicParserTest::testRule(true, NodeParsers::Instruction(), ".add .i64 (rax, 4), .i32 3452432;");
    BasicParserTest::expectChildCount(0, nodes);
}

TEST(TestInstructionParser, TestInvalidInstructionOperand2)
{
    auto nodes = BasicParserTest::testRule(true, NodeParsers::Instruction(), ".add .i64 (%rax, 4), .i32 ;");
    BasicParserTest::expectChildCount(0, nodes);
}