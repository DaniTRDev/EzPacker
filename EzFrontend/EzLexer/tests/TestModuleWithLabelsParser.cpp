#include "BasicParserTest.h"

TEST(TestModuleWithLabelsParser, TestValidModuleWithLabels)
{
    std::string moduleText = R"(.i32 myModule(.i64 %myVar, .i64 %myVar2)
                             {
                             testLabel1:
                               .add .i64 %rcx, .i64 %rbx;
                               .load .i64 (%rax, 4), .i64 %rbx;
                             testLabel2:
                             testLabel3:
                               .add .i64 %rcx, .i32 %rbx;
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

    BasicParserTest::expectChildNodeType(AstNodes::Label().getId(), 1, module);
    auto label1 = module->getChild(1);
    BasicParserTest::expectChildCount(3, label1);
    BasicParserTest::expectChildNodeType(AstNodes::Identifier().getId(), 0, label1);
    BasicParserTest::expectChildNodeType(AstNodes::Instruction().getId(), 1, label1);
    BasicParserTest::expectChildNodeType(AstNodes::Instruction().getId(), 2, label1);

    BasicParserTest::expectChildNodeType(AstNodes::Label().getId(), 2, module);
    auto label2 = module->getChild(2);
    BasicParserTest::expectChildCount(1, label2);
    BasicParserTest::expectChildNodeType(AstNodes::Identifier().getId(), 0, label2);

    BasicParserTest::expectChildNodeType(AstNodes::Label().getId(), 3, module);
    auto label3 = module->getChild(3);
    BasicParserTest::expectChildCount(2, label3);
    BasicParserTest::expectChildNodeType(AstNodes::Identifier().getId(), 0, label3);
    BasicParserTest::expectChildNodeType(AstNodes::Instruction().getId(), 1, label3);
}
