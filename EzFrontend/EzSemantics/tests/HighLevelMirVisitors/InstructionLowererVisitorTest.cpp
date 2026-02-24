#include "InstructionLowererVisitorTestFixture.h"

TEST_F(InstructionLowererVisitorTestFixture, TestLowerInstruction_Valid)
{
    std::string code = R"(
myLabel:
{
    create i32 %myVar;
    add %myVar, 123;
})";
    EXPECT_TRUE(runVisitor<LabelParser>(code));

    auto visitor = getVisitor();
    ASSERT_NE(visitor, nullptr);
    auto block = visitor->getCurrentBlock();
    ASSERT_NE(block, nullptr);

    EXPECT_FALSE(block->m_instructions.empty());
}

TEST_F(InstructionLowererVisitorTestFixture, TestLowerLabel_Valid)
{
    std::string code = R"(
myLabel:
{
    create i32 %myVar;
    add %myVar, 123;
    sub %myVar, 50;
})";

    EXPECT_TRUE(runVisitor<LabelParser>(code));

    auto visitor = getVisitor();
    ASSERT_NE(visitor, nullptr);
    auto block = visitor->getCurrentBlock();
    ASSERT_NE(block, nullptr);

    EXPECT_FALSE(block->m_instructions.empty());
}

TEST_F(InstructionLowererVisitorTestFixture, TestLowerModule_Valid)
{
    std::string code = R"(
void MyModule()
{
    create i32 %local;
    add %local, 10;
    ret i64 %local;
})";

    EXPECT_TRUE(runVisitor<ModuleParser::ModuleParser>(code));

    auto visitor = getVisitor();
    ASSERT_NE(visitor, nullptr);
    auto block = visitor->getCurrentBlock();
    ASSERT_NE(block, nullptr);

    EXPECT_FALSE(block->m_instructions.empty());
}

TEST_F(InstructionLowererVisitorTestFixture, TestLowerZeroOperandInstruction)
{
    std::string code = R"(
testLabel:
{
    nop;
})";
    EXPECT_TRUE(runVisitor<LabelParser>(code));
}

TEST_F(InstructionLowererVisitorTestFixture, TestLowerImmediateOperand_Integer)
{
    std::string code = R"(
testLabel:
{
    create i32 %var;
    add %var, 123;
})";
    EXPECT_TRUE(runVisitor<LabelParser>(code));
}

TEST_F(InstructionLowererVisitorTestFixture, TestLowerImmediateOperand_Float)
{
    std::string code = R"(
testLabel:
{
    create float %var;
    add %var, 123.456;
})";
    EXPECT_TRUE(runVisitor<LabelParser>(code));
}

TEST_F(InstructionLowererVisitorTestFixture, TestLowerMemoryOperand_BaseDisplacement)
{
    std::string code = R"(
testLabel:
{
    create i64 %base;
    create i32 %dest;
    load %dest, i32 (%base + 8);
})";
    EXPECT_TRUE(runVisitor<LabelParser>(code));
}

TEST_F(InstructionLowererVisitorTestFixture, TestLowerMemoryOperand_BaseIndexScaleDisplacement)
{
    std::string code = R"(
testLabel:
{
    create i64 %base;
    create i64 %index;
    create i32 %dest;
    load %dest, i32 (%base, %index, -4, -16);
})";
    EXPECT_TRUE(runVisitor<LabelParser>(code));
}

TEST_F(InstructionLowererVisitorTestFixture, TestLowerMemoryOperand_Direct)
{
    std::string code = R"(
testLabel:
{
    create i32 %dest;
    load %dest, i32 (%dest+0);
})";
    EXPECT_TRUE(runVisitor<LabelParser>(code));
}

TEST_F(InstructionLowererVisitorTestFixture, TestLowerMemoryOperand_IndexScale)
{
    std::string code = R"(
testLabel:
{
    create i64 %index;
    create i32 %dest;
    load %dest, i32 (, %index, 8);
})";
    EXPECT_TRUE(runVisitor<LabelParser>(code));
}

TEST_F(InstructionLowererVisitorTestFixture, TestLowerVariableOperand)
{
    std::string code = R"(
testLabel:
{
    create i32 %src;
    create i32 %dest;
    mov %dest, %src;
})";
    EXPECT_TRUE(runVisitor<LabelParser>(code));
}

TEST_F(InstructionLowererVisitorTestFixture, TestLowerLabelReference)
{
    std::string code = R"(
testLabel:
{
    jmp %testLabel;
})";
    EXPECT_TRUE(runVisitor<LabelParser>(code));
}

TEST_F(InstructionLowererVisitorTestFixture, TestLowerTypeCastVariable)
{
    std::string code = R"(
testLabel:
{
    create i32 %src;
    create i64 %dest;
    mov %dest, i64 (%src+0);
})";
    EXPECT_TRUE(runVisitor<LabelParser>(code));
}

TEST_F(InstructionLowererVisitorTestFixture, TestLowerInvalidOperandCount)
{
    std::string code = R"(
testLabel:
{
    create i32 %var;
    add i32 %var;
})";
    EXPECT_FALSE(runVisitor<LabelParser>(code));
}

TEST_F(InstructionLowererVisitorTestFixture, TestLowerInvalidMemoryScale)
{
    std::string code = R"(
testLabel:
{
    create i64 %base;
    create i64 %index;
    create i32 %dest;
    load %dest, i32 (%base, %index, 16, 0);
})";
    EXPECT_FALSE(runVisitor<LabelParser>(code));
}

TEST_F(InstructionLowererVisitorTestFixture, TestLowerBigIntegerImmediate)
{
    std::string code = R"(
testLabel:
{
    create i128 %var;
    add %var, i128 12312312312312321321312321312312312312312312312312321;
    store %var, (%var);
})";
    EXPECT_TRUE(runVisitor<LabelParser>(code));
}

TEST_F(InstructionLowererVisitorTestFixture, TestLowerNestedLabels)
{
    std::string code = R"(
parentLabel:
{
    create i32 %var;
    add %var, 1;
    childLabel:
    {
        sub %var, 1;
    }
})";
    EXPECT_TRUE(runVisitor<LabelParser>(code));

    auto visitor = getVisitor();
    auto block = visitor->getModuleBlock();
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->m_previous, nullptr);
    EXPECT_NE(block->m_next, nullptr);
    EXPECT_EQ(block->m_next->m_instructions.size(), 1);
}
