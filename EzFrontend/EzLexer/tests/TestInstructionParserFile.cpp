#include "BasicParserTest.h"

TEST(InstructionParseFileTest, TestFile)
{
    std::string fileContent = R"(
.load .i64 %tmp1, .i32 (%stackFrame, 0);
.pop .i8 %stackFrame;
.freeStack .i8 32;
.push .i64 %rcx;
.add .i32 %rcx, .i32 %rcx;
.load .i32 %tmp2, .i64 (%stackFrame, 8);
.sub .i64 %tmp1, .i16 %tmp2;
.mul .i32 %rax, .i8 %rbx;
.store .i64 %tmp1, .i16 (%stackFrame, 16);
.push .i8 %rflags;
.and .i8 %rflags, .i128 %rflags;)";

    auto nodes = BasicParserTest::testRule(false, Rules::ManyOf(NodeParsers::Instruction()), fileContent);
    EXPECT_EQ(nodes->getChildren().size(), 11);

    for (auto &instr : nodes->getChildren())
    {
        auto instrKeyword = instr->getChild(0);
        std::cout << "Recognised " << instrKeyword->getContent() << std::endl;
    }
}
