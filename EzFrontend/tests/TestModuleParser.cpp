#include "BasicParserTest.h"

TEST(TestModuleParser, TestValidModule)
{
    std::string moduleText = ".module myModule(.i64 %myVar, .i64 %myVar2)"
                             "  .add .i64 %rcx, .i64 %rbx"
                             "  .load .i64 (, %rax, 4), .i64 %rbx"
                             ".end";

    auto nodes = BasicParserTest::testRule(false, grammar::module(), moduleText);
    EXPECT_EQ(nodes->getChildren().size(), 1);
    EXPECT_EQ(nodes->getChildren()[0]->getType(), AstType::Module);

    auto module = std::dynamic_pointer_cast<ModuleNode>(nodes->getChildren()[0]);
    EXPECT_EQ(module->getChildren().size(), 7);
    EXPECT_EQ(module->getChildren()[0]->getType(), AstType::Identifier);
    EXPECT_EQ(module->getChildren()[1]->getType(), AstType::Identifier);
    EXPECT_EQ(module->getChildren()[2]->getType(), AstType::ModuleParameter);
    EXPECT_EQ(module->getChildren()[3]->getType(), AstType::ModuleParameter);

    EXPECT_EQ(module->getChildren()[4]->getType(), AstType::Instruction);
    EXPECT_EQ(module->getChildren()[5]->getType(), AstType::Instruction);

    EXPECT_EQ(module->getChildren()[module->getChildren().size() - 1]->getType(), AstType::Identifier);
}

TEST(TestModuleParser, TestValidModule2)
{
    std::string moduleText = ".module _asdadasda_()"
                             "  .add .i64 %rcx, .i64 %rbx"
                             "  .load .i64 (, %rax, 4), .i64 %rbx"
                             "  .xchg .i64 (, %rax, 4), .i64 %rbx"
                             ".end";

    auto nodes = BasicParserTest::testRule(false, grammar::module(), moduleText);
    EXPECT_EQ(nodes->getChildren().size(), 1);
    EXPECT_EQ(nodes->getChildren()[0]->getType(), AstType::Module);

    auto module = std::dynamic_pointer_cast<ModuleNode>(nodes->getChildren()[0]);
    EXPECT_EQ(module->getChildren().size(), 6);
    EXPECT_EQ(module->getChildren()[0]->getType(), AstType::Identifier);
    EXPECT_EQ(module->getChildren()[1]->getType(), AstType::Identifier);

    EXPECT_EQ(module->getChildren()[2]->getType(), AstType::Instruction);
    EXPECT_EQ(module->getChildren()[3]->getType(), AstType::Instruction);
    EXPECT_EQ(module->getChildren()[4]->getType(), AstType::Instruction);

    EXPECT_EQ(module->getChildren()[module->getChildren().size() - 1]->getType(), AstType::Identifier);
}

TEST(TestModuleParser, TestInvalidKeyword)
{
    std::string moduleText = "module _asdadasda_()"
                             "  .add .i64 %rcx, .i64 %rbx"
                             "  .load .i64 (, %rax, 4), .i64 %rbx"
                             "  .xchg .i64 (, %rax, 4), .i64 %rbx"
                             ".end";

    auto nodes = BasicParserTest::testRule(true, grammar::module(), moduleText);
    EXPECT_EQ(nodes->getChildren().size(), 0);
}

TEST(TestModuleParser, TestInvalidKeyword2)
{
    std::string moduleText = ".module _asdadasda_()"
                             "  .add .i64 %rcx, .i64 %rbx"
                             "  .load .i64 (, %rax, 4), .i64 %rbx"
                             "  .xchg .i64 (, %rax, 4), .i64 %rbx"
                             "end";

    auto nodes = BasicParserTest::testRule(true, grammar::module(), moduleText);
    EXPECT_EQ(nodes->getChildren().size(), 0);
}

TEST(TestModuleParser, TestInvalidName)
{
    std::string moduleText = ".module ()"
                             "  .add .i64 %rcx, .i64 %rbx"
                             "  .load .i64 (, %rax, 4), .i64 %rbx"
                             "  .xchg .i64 (, %rax, 4), .i64 %rbx"
                             "end";

    auto nodes = BasicParserTest::testRule(true, grammar::module(), moduleText);
    EXPECT_EQ(nodes->getChildren().size(), 0);
}

TEST(TestModuleParser, TestInvalidBody)
{
    std::string moduleText = ".module myModule()"
                             " .add i64 %rcx"
                             ".end";

    auto nodes = BasicParserTest::testRule(true, grammar::module(), moduleText);
    EXPECT_EQ(nodes->getChildren().size(), 0);
}

TEST(TestModuleParser, TestInvalidBody2)
{
    std::string moduleText = ".module myModule()"
                             " .add"
                             ".end";

    auto nodes = BasicParserTest::testRule(true, grammar::module(), moduleText);
    EXPECT_EQ(nodes->getChildren().size(), 0);
}