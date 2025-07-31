#include "BasicParserTest.h"

TEST(ModuleParserTestFile, TestFile)
{
    std::string fileContent = R"(
.i32 mathOps(.i32 %rbx, .i64 %rax)
{
  .add .i64 %rcx, .i64 %rbx;
  .sub .i64 %rcx, .i64 %rax;
  .load .i64 (, %rsi, 8), .i64 %rbx;
  .store .i64 %rbx, .i64 (, %rsi, 8);
}

.i32 stackHandling()
{
  .push .i64 %rax;
  .pop .i64 %rbx;
  .xchg .i64 (, %rdi, 4), .i64 %rbx;
  .xchg .i32 (%rbp, 0), .i64 %rcx;
}

.i32 memAccess()
{
  .load .i32 (%rbp, 16), .i64 %r10d;
  .store .i32 %r10d, .i64 (%rbp, 16);
  .load .i64 (, %r9, 2), .i64 %r11;
  .store .i64 %r11, .i64 (, %r9, 2);
}

.i32 cleanup()
{
  .freeStack .i8 32;
  .pop .i8 %flags;
  .push .i64 %r8;
  .xchg .i64 (%rsp, 0), .i64 %r8;
}

.i32 trickyLoad()
{
  .load .i64 (, %rax, 4), .i64 %rbx;
  .xchg .i64 (, %rax, 4), .i64 %rbx;
  .store .i64 %rbx, .i64 (, %rax, 4);
})";

    auto result = BasicParserTest::testRule(false, Rules::ManyOf(NodeParsers::Module()), fileContent);
    BasicParserTest::expectChildCount(5, result);

    for (auto &var : result->getChildren())
    {
        auto moduleHeader = var->getChild(0);
        auto moduleIdentifier = moduleHeader->getChild(1);

        std::cout << "Recognised " << moduleIdentifier->getContent() << std::endl;
    }
}