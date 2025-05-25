#include "BasicParserTest.h"
#include "Semantics/Normalizer/InstructionNormalizer.h"

TEST(TestInstructionParser, TestValidInstruction)
{
    auto nodes = BasicParserTest::testRule(false, grammar::instruction(), ".add .i64 %rcx, .i32 (, %rcx, 4)");
    EXPECT_EQ(nodes->getChildren().size(), 1);

    auto instr = std::dynamic_pointer_cast<InstructionNode>(nodes->getChildren()[0]);
    EXPECT_EQ(instr->getChildren().size(), 5);
    EXPECT_EQ(instr->getChildren()[0]->getType(), AstType::Identifier);
    EXPECT_EQ(instr->getChildren()[1]->getType(), AstType::Type);
    EXPECT_EQ(instr->getChildren()[2]->getType(), AstType::VirtualVariable);
    EXPECT_EQ(instr->getChildren()[3]->getType(), AstType::Type);
    EXPECT_EQ(instr->getChildren()[4]->getType(), AstType::Memory);

    auto memory = std::dynamic_pointer_cast<MemoryNode>(instr->getChildren()[4]);
    EXPECT_EQ(memory->getChildren().size(), 2);
    EXPECT_EQ(memory->getChildren()[0]->getType(), AstType::VirtualVariable);
    EXPECT_EQ(memory->getChildren()[1]->getType(), AstType::Value);
}

TEST(TestInstructionParser, TestValidInstruction2)
{
    auto nodes = BasicParserTest::testRule(false, grammar::instruction(), ".add .i64 %rcx, .i64 %rcx");
    EXPECT_EQ(nodes->getChildren().size(), 1);

    auto instr = std::dynamic_pointer_cast<InstructionNode>(nodes->getChildren()[0]);
    EXPECT_EQ(instr->getChildren().size(), 5);
    EXPECT_EQ(instr->getChildren()[0]->getType(), AstType::Identifier);
    EXPECT_EQ(instr->getChildren()[1]->getType(), AstType::Type);
    EXPECT_EQ(instr->getChildren()[2]->getType(), AstType::VirtualVariable);
    EXPECT_EQ(instr->getChildren()[3]->getType(), AstType::Type);
    EXPECT_EQ(instr->getChildren()[4]->getType(), AstType::VirtualVariable);
}

TEST(TestInstructionParser, TestValidInstruction3)
{
    auto nodes = BasicParserTest::testRule(false, grammar::instruction(), ".add .i64 %rcx, .i64 123423");
    EXPECT_EQ(nodes->getChildren().size(), 1);

    auto instr = std::dynamic_pointer_cast<InstructionNode>(nodes->getChildren()[0]);
    EXPECT_EQ(instr->getChildren().size(), 5);
    EXPECT_EQ(instr->getChildren()[0]->getType(), AstType::Identifier);
    EXPECT_EQ(instr->getChildren()[1]->getType(), AstType::Type);
    EXPECT_EQ(instr->getChildren()[2]->getType(), AstType::VirtualVariable);
    EXPECT_EQ(instr->getChildren()[3]->getType(), AstType::Type);
    EXPECT_EQ(instr->getChildren()[4]->getType(), AstType::Value);
}

TEST(TestInstructionParser, TestValidInstruction4)
{
    auto nodes = BasicParserTest::testRule(false, grammar::instruction(), ".add .i32 (, %rcx, 3), .i64 %rcx");
    EXPECT_EQ(nodes->getChildren().size(), 1);

    auto instr = std::dynamic_pointer_cast<InstructionNode>(nodes->getChildren()[0]);
    EXPECT_EQ(instr->getChildren().size(), 5);
    EXPECT_EQ(instr->getChildren()[0]->getType(), AstType::Identifier);
    EXPECT_EQ(instr->getChildren()[1]->getType(), AstType::Type);
    EXPECT_EQ(instr->getChildren()[2]->getType(), AstType::Memory);
    EXPECT_EQ(instr->getChildren()[3]->getType(), AstType::Type);
    EXPECT_EQ(instr->getChildren()[4]->getType(), AstType::VirtualVariable);

    auto memory = std::dynamic_pointer_cast<MemoryNode>(instr->getChildren()[2]);
    EXPECT_EQ(memory->getChildren().size(), 2);
    EXPECT_EQ(memory->getChildren()[0]->getType(), AstType::VirtualVariable);
    EXPECT_EQ(memory->getChildren()[1]->getType(), AstType::Value);
}

TEST(TestInstructionParser, TestValidInstruction5)
{
    auto nodes = BasicParserTest::testRule(false, grammar::instruction(), ".add .i32 (%rax, 4), .i32 3452432");
    EXPECT_EQ(nodes->getChildren().size(), 1);

    auto instr = std::dynamic_pointer_cast<InstructionNode>(nodes->getChildren()[0]);
    EXPECT_EQ(instr->getChildren().size(), 5);
    EXPECT_EQ(instr->getChildren()[0]->getType(), AstType::Identifier);
    EXPECT_EQ(instr->getChildren()[1]->getType(), AstType::Type);
    EXPECT_EQ(instr->getChildren()[2]->getType(), AstType::Memory);
    EXPECT_EQ(instr->getChildren()[3]->getType(), AstType::Type);
    EXPECT_EQ(instr->getChildren()[4]->getType(), AstType::Value);

    auto memory = std::dynamic_pointer_cast<MemoryNode>(instr->getChildren()[2]);
    EXPECT_EQ(memory->getChildren().size(), 2);
    EXPECT_EQ(memory->getChildren()[0]->getType(), AstType::VirtualVariable);
    EXPECT_EQ(memory->getChildren()[1]->getType(), AstType::Value);
}

TEST(TestInstructionParser, TestValidInstructionSingleOperand)
{
    auto nodes = BasicParserTest::testRule(false, grammar::instruction(), ".push .i32 (%rax, 4)");
    EXPECT_EQ(nodes->getChildren().size(), 1);

    auto instr = std::dynamic_pointer_cast<InstructionNode>(nodes->getChildren()[0]);
    EXPECT_EQ(instr->getChildren().size(), 3);
    EXPECT_EQ(instr->getChildren()[0]->getType(), AstType::Identifier);
    EXPECT_EQ(instr->getChildren()[1]->getType(), AstType::Type);
    EXPECT_EQ(instr->getChildren()[2]->getType(), AstType::Memory);

    auto memory = std::dynamic_pointer_cast<MemoryNode>(instr->getChildren()[2]);
    EXPECT_EQ(memory->getChildren().size(), 2);
    EXPECT_EQ(memory->getChildren()[0]->getType(), AstType::VirtualVariable);
    EXPECT_EQ(memory->getChildren()[1]->getType(), AstType::Value);
}

TEST(TestInstructionParser, TestInvalidInstructionName)
{
    auto nodes = BasicParserTest::testRule(true, grammar::instruction(), "add .i64 (%rax, 4), .i32 3452432");
    EXPECT_EQ(nodes->getChildren().size(), 0);
}

TEST(TestInstructionParser, TestInvalidInstructionType)
{
    auto nodes = BasicParserTest::testRule(true, grammar::instruction(), ".add i64 (%rax, 4), .i32 3452432");
    EXPECT_EQ(nodes->getChildren().size(), 0);
}

TEST(TestInstructionParser, TestInvalidInstructionOperand1)
{
    auto nodes = BasicParserTest::testRule(true, grammar::instruction(), ".add .i64 (rax, 4), .i32 3452432");
    EXPECT_EQ(nodes->getChildren().size(), 0);
}

TEST(TestInstructionParser, TestInvalidInstructionOperand2)
{
    auto nodes = BasicParserTest::testRule(true, grammar::instruction(), ".add .i64 (%rax, 4), .i32 ");
    EXPECT_EQ(nodes->getChildren().size(), 0);
}