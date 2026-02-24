#include "ParsersTestFixture.h"

TEST_F(ParsersTestFixture, LabelEmpty)
{
    std::string input = "myLabel:{}";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<LabelParser>());
}

TEST_F(ParsersTestFixture, Label1Instr)
{
    std::string input = R"(
myLabel:
{
    add %myVar, 123;
})";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<LabelParser>());
    TEST_LABEL("myLabel", AstNodeType::Instruction);
}

TEST_F(ParsersTestFixture, Label2Instr)
{
    std::string input = R"(
myLabel:
{
    add %myVar, 123;
    nop;
})";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<LabelParser>());
    TEST_LABEL("myLabel", AstNodeType::Instruction, AstNodeType::Instruction);
}

TEST_F(ParsersTestFixture, LabelMultiple2Instr)
{
    std::string input = R"(
myLabel:
{
    add %myVar2, i64 (1234);
    add %myVar3, i32 (%base+0);
    add %myVar4, i16 (%base + 123);
    nop;
})";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<LabelParser>());
    TEST_LABEL("myLabel",
               AstNodeType::Instruction,
               AstNodeType::Instruction,
               AstNodeType::Instruction,
               AstNodeType::Instruction);
}

TEST_F(ParsersTestFixture, LabelInvalidInstr)
{
    std::string input = R"(
myLabel:
{
    ;
})";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<LabelParser>());
}

TEST_F(ParsersTestFixture, LabelNInstrNestedLabel)
{
    std::string input = R"(
myLabel:
{
    add i8 %myVar, 1;
    myLabel2:
    {
        add %myVar2, i64 (1234);
        add %myVar3, i32 (%base+0);
        add %myVar4, i16 (%base      +       123);
    }
})";
    tokenizeAndCreateContext(input);
    EXPECT_TRUE(expectParse<LabelParser>());
    TEST_LABEL("myLabel", AstNodeType::Instruction, AstNodeType::Label);
}

TEST_F(ParsersTestFixture, LabelMissingColon)
{
    std::string input = "myLabel {}";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<LabelParser>());
}

TEST_F(ParsersTestFixture, LabelMissingLeftBrace)
{
    std::string input = "myLabel: ";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<LabelParser>());
}

TEST_F(ParsersTestFixture, LabelMissingRightBrace)
{
    std::string input = "myLabel: { nop; ";
    tokenizeAndCreateContext(input);
    EXPECT_FALSE(expectParse<LabelParser>());
}
