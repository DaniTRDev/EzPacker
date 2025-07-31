#include "BasicParserTest.h"

TEST(TestModuleParser, TestValidModule)
{
    std::string moduleText = R"(.i32 myModule(.i64 %myVar, .i64 %myVar2)
                             {
                               .add .i64 %rcx, .i64 %rbx;
                               .load .i64 (%rax, 4), .i64 %rbx;
                             })";

    auto result = BasicParserTest::testRule(false, NodeParsers::Module(), moduleText);
    BasicParserTest::expectChildCount(1, result);
    BasicParserTest::expectChildNodeType(AstNodes::Module().getId(), 0, result);

    auto module = result->getChild(0);
    auto moduleHeader = module->getChild(0);
    BasicParserTest::expectChildCount(6, moduleHeader);
    BasicParserTest::expectChildNodeType(AstNodes::Type().getId(), 0, moduleHeader);
    BasicParserTest::expectChildNodeType(AstNodes::Identifier().getId(), 1, moduleHeader);
    BasicParserTest::expectChildNodeType(AstNodes::Type().getId(), 2, moduleHeader);
    BasicParserTest::expectChildNodeType(AstNodes::VirtualVariable().getId(), 3, moduleHeader);
    BasicParserTest::expectChildNodeType(AstNodes::Type().getId(), 4, moduleHeader);
    BasicParserTest::expectChildNodeType(AstNodes::VirtualVariable().getId(), 5, moduleHeader);

    BasicParserTest::expectChildNodeType(AstNodes::Instruction().getId(), 1, module);
    BasicParserTest::expectChildNodeType(AstNodes::Instruction().getId(), 2, module);
}

TEST(TestModuleParser, TestValidModule2)
{
    std::string moduleText = R"(.i32 _asdadasda_()
                             {
                               .add .i64 %rcx, .i64 %rbx;
                               .load .i64 (, %rax, 4), .i64 %rbx;
                               .xchg .i64 (, %rax, 4), .i64 %rbx;
                             })";

    auto result = BasicParserTest::testRule(false, NodeParsers::Module(), moduleText);
    BasicParserTest::expectChildCount(1, result);
    BasicParserTest::expectChildNodeType(AstNodes::Module().getId(), 0, result);

    auto module = result->getChild(0);
    auto moduleHeader = module->getChild(0);
    BasicParserTest::expectChildCount(2, moduleHeader);
    BasicParserTest::expectChildNodeType(AstNodes::Type().getId(), 0, moduleHeader);
    BasicParserTest::expectChildNodeType(AstNodes::Identifier().getId(), 1, moduleHeader);

    BasicParserTest::expectChildNodeType(AstNodes::Instruction().getId(), 1, module);
    BasicParserTest::expectChildNodeType(AstNodes::Instruction().getId(), 2, module);
    BasicParserTest::expectChildNodeType(AstNodes::Instruction().getId(), 3, module);
}

TEST(TestModuleParser, TestInvalidBrace)
{
    std::string moduleText = R"(.i32 _asdadasda_()
                               .add .i64 %rcx, .i64 %rbx;
                               .load .i64 (, %rax, 4), .i64 %rbx;
                               .xchg .i64 (%rax), .i64 %rbx;
                             })";

    auto result = BasicParserTest::testRule(true, NodeParsers::Module(), moduleText);
    BasicParserTest::expectChildCount(0, result);
}

TEST(TestModuleParser, TestInvalidBrace2)
{
    std::string moduleText = R"(.i32 _asdadasda_()
                             {
                               .add .i64 %rcx, .i64 %rbx;
                               .load .i64 (, %rax, 4), .i64 %rbx;
                               .xchg .i64 (, %rax, 4), .i64 %rbx;
                             )";

    auto result = BasicParserTest::testRule(true, NodeParsers::Module(), moduleText);
    BasicParserTest::expectChildCount(0, result);
}

TEST(TestModuleParser, TestInvalidName)
{
    std::string moduleText = R"(.i32 ()
                             {
                               .add .i64 %rcx, .i64 %rbx;
                               .load .i64 (4), .i64 %rbx;
                               .xchg .i64 (%rbx, %rax, 4), .i64 %rbx;
                             })";

    auto result = BasicParserTest::testRule(true, NodeParsers::Module(), moduleText);
    BasicParserTest::expectChildCount(0, result);
}

TEST(TestModuleParser, TestInvalidBody)
{
    std::string moduleText = R"(.i32 myModule()
                             {
                              .add i64 %rcx;
                             })";

    auto result = BasicParserTest::testRule(true, NodeParsers::Module(), moduleText);
    BasicParserTest::expectChildCount(0, result);
}

TEST(TestModuleParser, TestInvalidBody2)
{
    std::string moduleText = R"(.i32 myModule()
                             "{"
                             " ;"
                             })";

    auto result = BasicParserTest::testRule(true, NodeParsers::Module(), moduleText);
    BasicParserTest::expectChildCount(0, result);
}