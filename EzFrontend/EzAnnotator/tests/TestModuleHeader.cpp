#include "BasicAnnotatorTest.h"

/*
 * Module tests are divided in 2 files for better testing. This one will only test that the header was correctly
 * annotated.
 */

TEST_F(BasicAnnotatorTest, TestModuleSymbol)
{
    // This test will only assure that module's symbol was correctly annotated.
    std::string input = R"(
.i32 mathOps()
{
    # Empty!
})";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Module(), input);
    std::shared_ptr<ModuleAnnotator> mmAnnotator = std::make_shared<ModuleAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    size_t symbolId = 0;
    std::shared_ptr<AstNode> moduleNode = node->getChild(0);

    EXPECT_TRUE(mmAnnotator->annotate(moduleNode, m_logger));
    EXPECT_TRUE(
            m_symbolTable->doesElemExistAtAnyUpperScopeByString(ScopedSymbol::getStringFromName("mathOps"), symbolId));
    EXPECT_TRUE(expectModuleReturnType(m_i32TypeId, moduleNode));
}

TEST_F(BasicAnnotatorTest, TestModuleHeaderNoArgument)
{
    // This module will test that a module has no annotated arguments.
    std::string input = R"(
.i32 mathOps()
{
    # Empty!
})";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Module(), input);
    std::shared_ptr<ModuleAnnotator> mmAnnotator = std::make_shared<ModuleAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    std::shared_ptr<AstNode> moduleNode = node->getChild(0);

    EXPECT_TRUE(mmAnnotator->annotate(moduleNode, m_logger));
    EXPECT_TRUE(expectModuleArgumentCount(0, moduleNode));
}

TEST_F(BasicAnnotatorTest, TestModuleHeaderSingleArgument)
{
    // This module will test that a module has a single argument, and it is correctly annotated.
    std::string input = R"(
.i32 mathOps(.i8 %arg1)
{
    # Empty!
})";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Module(), input);
    std::shared_ptr<ModuleAnnotator> mmAnnotator = std::make_shared<ModuleAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    std::shared_ptr<AstNode> moduleNode = node->getChild(0);

    EXPECT_TRUE(mmAnnotator->annotate(moduleNode, m_logger));
    EXPECT_TRUE(expectModuleArgumentCount(1, moduleNode));

    std::shared_ptr<ModuleAnnotation> annot = std::dynamic_pointer_cast<ModuleAnnotation>(moduleNode->getAnnotation());
    size_t argSymbolId = annot->getArgumentSymbolId(0);

    EXPECT_NE(argSymbolId, 0);
    EXPECT_TRUE(expectModuleArgument(argSymbolId, m_i8TypeId, 0, moduleNode));
}

TEST_F(BasicAnnotatorTest, TestModuleHeaderMultipleArgument)
{
    std::string input = R"(
.i32 mathOps(.i8 %arg1, .i16 %arg2, .i32 %arg3, .i64 %arg4)
{
    # Empty!
})";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Module(), input);
    std::shared_ptr<ModuleAnnotator> mmAnnotator = std::make_shared<ModuleAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    std::shared_ptr<AstNode> moduleNode = node->getChild(0);

    EXPECT_TRUE(mmAnnotator->annotate(moduleNode, m_logger));
    EXPECT_TRUE(expectModuleArgumentCount(4, moduleNode));

    std::shared_ptr<ModuleAnnotation> annot = std::dynamic_pointer_cast<ModuleAnnotation>(moduleNode->getAnnotation());
    size_t arg1SymbolId = annot->getArgumentSymbolId(0), arg2SymbolId = annot->getArgumentSymbolId(1),
           arg3SymbolId = annot->getArgumentSymbolId(2), arg4SymbolId = annot->getArgumentSymbolId(3);

    EXPECT_NE(arg1SymbolId, 0);
    EXPECT_NE(arg2SymbolId, 0);
    EXPECT_NE(arg3SymbolId, 0);
    EXPECT_NE(arg4SymbolId, 0);
    EXPECT_TRUE(expectModuleArgument(arg1SymbolId, m_i8TypeId, 0, moduleNode));
    EXPECT_TRUE(expectModuleArgument(arg2SymbolId, m_i16TypeId, 1, moduleNode));
    EXPECT_TRUE(expectModuleArgument(arg3SymbolId, m_i32TypeId, 2, moduleNode));
    EXPECT_TRUE(expectModuleArgument(arg4SymbolId, m_i64TypeId, 3, moduleNode));
}

TEST_F(BasicAnnotatorTest, TestModuleHeaderInvalidReturnType)
{
    std::string input = R"(
.i80 mathOps(.i8 %arg1, .i16 %arg2, .i32 %arg3, .i64 %arg4)
{
    # Empty!
})";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Module(), input);
    std::shared_ptr<ModuleAnnotator> mmAnnotator = std::make_shared<ModuleAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    std::shared_ptr<AstNode> moduleNode = node->getChild(0);
    EXPECT_FALSE(mmAnnotator->annotate(moduleNode, m_logger));
}

TEST_F(BasicAnnotatorTest, TestModuleHeaderInvalidArgType)
{
    std::string input = R"(
.i32 mathOps(.i8 %arg1, .i16 %arg2, .i80 %arg3, .i64 %arg4)
{
    # Empty!
})";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Module(), input);
    std::shared_ptr<ModuleAnnotator> mmAnnotator = std::make_shared<ModuleAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    std::shared_ptr<AstNode> moduleNode = node->getChild(0);
    EXPECT_FALSE(mmAnnotator->annotate(moduleNode, m_logger));
}

TEST_F(BasicAnnotatorTest, TestModuleHeaderDupplicatedArgName)
{
    std::string input = R"(
.i32 mathOps(.i8 %arg1, .i16 %arg2, .i80 %arg2, .i64 %arg4)
{
    # Empty!
})";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Module(), input);
    std::shared_ptr<ModuleAnnotator> mmAnnotator = std::make_shared<ModuleAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    std::shared_ptr<AstNode> moduleNode = node->getChild(0);
    EXPECT_FALSE(mmAnnotator->annotate(moduleNode, m_logger));
}