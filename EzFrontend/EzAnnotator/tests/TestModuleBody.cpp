#include "BasicAnnotatorTest.h"

/*
 * Module tests are divided in 2 files for better testing. This one will only test the body of a module.
 */

TEST_F(BasicAnnotatorTest, TestModuleBody1Instruction)
{
    std::string input = R"(
.i32 myModule(.i16 %arg1, .i32 %arg2, .i64 %arg3)
{
    .add .i64 %arg1, .i16 %arg2; # This is be fine. This type of """errors""" should be handled with constraints.
})";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Module(), input);
    std::shared_ptr<ModuleAnnotator> mmAnnotator = std::make_shared<ModuleAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    std::shared_ptr<AstNode> moduleNode = node->getChild(0);
    EXPECT_TRUE(mmAnnotator->annotate(moduleNode, m_logger));
    EXPECT_TRUE(expectInstrInNode(0, "add", moduleNode));
}

TEST_F(BasicAnnotatorTest, TestModuleBodyNInstructions)
{
    std::string input = R"(
.i32 myModule(.i16 %arg1, .i32 %arg2, .i64 %arg3)
{
    .add .i64 %arg1, .i16 15;
    .sub .i64 %arg3, .float 3.5;
    .xor .i64 %arg1, .i16 %arg3;
    .not .i64 %arg1, .i16 %arg2;
})";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Module(), input);
    std::shared_ptr<ModuleAnnotator> mmAnnotator = std::make_shared<ModuleAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    std::shared_ptr<AstNode> moduleNode = node->getChild(0);
    EXPECT_TRUE(mmAnnotator->annotate(moduleNode, m_logger));
    EXPECT_TRUE(expectInstrInNode(0, "add", moduleNode));
    EXPECT_TRUE(expectInstrInNode(1, "sub", moduleNode));
    EXPECT_TRUE(expectInstrInNode(2, "xor", moduleNode));
    EXPECT_TRUE(expectInstrInNode(3, "not", moduleNode));
}

TEST_F(BasicAnnotatorTest, TestModuleBodyNInstructionsInvalidSymbol)
{
    std::string input = R"(
.i32 myModule(.i16 %arg1, .i32 %arg2, .i64 %arg3)
{
    .add .i64 %arg1, .i16 15;
    .sub .i64 %arg3, .float 3.5;
    .xor .i64 %arg1, .i16 %arg8;
    .not .i64 %arg1, .i16 %arg2;
})";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Module(), input);
    std::shared_ptr<ModuleAnnotator> mmAnnotator = std::make_shared<ModuleAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    std::shared_ptr<AstNode> moduleNode = node->getChild(0);
    EXPECT_FALSE(mmAnnotator->annotate(moduleNode, m_logger));
}

TEST_F(BasicAnnotatorTest, TestModuleBody1Label1Instr)
{
    std::string input = R"(
.i32 myModule(.i16 %arg1, .i32 %arg2, .i64 %arg3)
{
    testLabel:
        .add .i64 %arg1, .i16 15;
})";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Module(), input);
    std::shared_ptr<ModuleAnnotator> mmAnnotator = std::make_shared<ModuleAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    const std::shared_ptr<AstNode> &moduleNode = node->getChild(0);
    EXPECT_TRUE(mmAnnotator->annotate(moduleNode, m_logger));

    const std::shared_ptr<AstNode> &testLabelNode = moduleNode->getChild(0);
    EXPECT_TRUE(expectInstrInNode(0, "add", testLabelNode));
}

TEST_F(BasicAnnotatorTest, TestModuleBody1LabelRedefinition1Instr)
{
    std::string input = R"(
.i32 myModule(.i16 %arg1, .i32 %arg2, .i64 %arg3)
{
    arg1:
        .add .i64 %arg1, .i16 15;
})";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Module(), input);
    std::shared_ptr<ModuleAnnotator> mmAnnotator = std::make_shared<ModuleAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    std::shared_ptr<AstNode> moduleNode = node->getChild(0);
    EXPECT_FALSE(mmAnnotator->annotate(moduleNode, m_logger));
}

TEST_F(BasicAnnotatorTest, TestModuleBody1LabelNInstr)
{
    std::string input = R"(
.i32 myModule(.i16 %arg1, .i32 %arg2, .i64 %arg3)
{
    testLabel:
        .add .i64 %arg1, .i16 15;
        .sub .i64 %arg3, .float 3.5;
        .xor .i64 %arg1, .i16 %arg3;
        .not .i64 %arg1, .i16 %arg2;
})";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Module(), input);
    std::shared_ptr<ModuleAnnotator> mmAnnotator = std::make_shared<ModuleAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    std::shared_ptr<AstNode> moduleNode = node->getChild(0);
    EXPECT_TRUE(mmAnnotator->annotate(moduleNode, m_logger));

    size_t labelId = 0;
    const std::shared_ptr<AstNode> &testLabelNode = moduleNode->getChild(0);

    EXPECT_TRUE(m_symbolTable->doesElemExistAtAnyScopeByString(ScopedSymbol::getStringFromName("testLabel"), labelId));
    EXPECT_TRUE(expectModuleLabelInBody(0, labelId, moduleNode));
    EXPECT_TRUE(expectInstrInNode(0, "add", testLabelNode));
    EXPECT_TRUE(expectInstrInNode(1, "sub", testLabelNode));
    EXPECT_TRUE(expectInstrInNode(2, "xor", testLabelNode));
    EXPECT_TRUE(expectInstrInNode(3, "not", testLabelNode));
}

TEST_F(BasicAnnotatorTest, TestModuleBodyNLabel1Instr)
{
    std::string input = R"(
.i32 myModule(.i16 %arg1, .i32 %arg2, .i64 %arg3)
{
    testLabel:
        .add .i64 %arg1, .i16 15;
    testLabel2:
        .sub .i64 %arg1, .i16 15;
    testLabel3:
        .not .i64 %arg1, .i16 15;
})";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Module(), input);
    std::shared_ptr<ModuleAnnotator> mmAnnotator = std::make_shared<ModuleAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    std::shared_ptr<AstNode> moduleNode = node->getChild(0);
    EXPECT_TRUE(mmAnnotator->annotate(moduleNode, m_logger));

    size_t label1Id = 0, label2Id = 0, label3Id = 0;
    const std::shared_ptr<AstNode> &label1Node = moduleNode->getChild(0), &label2Node = moduleNode->getChild(1),
                                   &label3Node = moduleNode->getChild(2);

    EXPECT_TRUE(m_symbolTable->doesElemExistAtAnyScopeByString(ScopedSymbol::getStringFromName("testLabel"), label1Id));
    EXPECT_TRUE(
            m_symbolTable->doesElemExistAtAnyScopeByString(ScopedSymbol::getStringFromName("testLabel2"), label2Id));
    EXPECT_TRUE(
            m_symbolTable->doesElemExistAtAnyScopeByString(ScopedSymbol::getStringFromName("testLabel3"), label3Id));

    EXPECT_TRUE(expectModuleLabelInBody(0, label1Id, moduleNode));
    EXPECT_TRUE(expectModuleLabelInBody(1, label2Id, moduleNode));
    EXPECT_TRUE(expectModuleLabelInBody(2, label3Id, moduleNode));

    EXPECT_TRUE(expectInstrInNode(0, "add", label1Node));
    EXPECT_TRUE(expectInstrInNode(0, "sub", label2Node));
    EXPECT_TRUE(expectInstrInNode(0, "not", label3Node));
}

TEST_F(BasicAnnotatorTest, TestModuleBodyNLabelNInstr)
{
    std::string input = R"(
.i32 myModule(.i16 %arg1, .i32 %arg2, .i64 %arg3)
{
    testLabel:
        .add .i64 %arg1, .i16 15;
        .jmp .i64 %arg1, .i16 15;
        .store .i64 %arg1, .i16 15;
    testLabel2:
        .sub .i64 %arg1, .i16 15;
        .xor .i64 %arg1, .i16 15;
        .and .i64 %arg1, .i16 15;
    testLabel3:
        .not .i64 %arg1, .i16 15;
        .or .i64 %arg1, .i16 15;
        .xor .i64 %arg1, .i16 15;
})";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Module(), input);
    std::shared_ptr<ModuleAnnotator> mmAnnotator = std::make_shared<ModuleAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    std::shared_ptr<AstNode> moduleNode = node->getChild(0);
    EXPECT_TRUE(mmAnnotator->annotate(moduleNode, m_logger));

    size_t label1Id = 0, label2Id = 0, label3Id = 0;
    const std::shared_ptr<AstNode> &label1Node = moduleNode->getChild(0), &label2Node = moduleNode->getChild(1),
                                   &label3Node = moduleNode->getChild(2);

    EXPECT_TRUE(m_symbolTable->doesElemExistAtAnyScopeByString(ScopedSymbol::getStringFromName("testLabel"), label1Id));
    EXPECT_TRUE(
            m_symbolTable->doesElemExistAtAnyScopeByString(ScopedSymbol::getStringFromName("testLabel2"), label2Id));
    EXPECT_TRUE(
            m_symbolTable->doesElemExistAtAnyScopeByString(ScopedSymbol::getStringFromName("testLabel3"), label3Id));

    EXPECT_TRUE(expectModuleLabelInBody(0, label1Id, moduleNode));
    EXPECT_TRUE(expectModuleLabelInBody(1, label2Id, moduleNode));
    EXPECT_TRUE(expectModuleLabelInBody(2, label3Id, moduleNode));

    EXPECT_TRUE(expectInstrInNode(0, "add", label1Node));
    EXPECT_TRUE(expectInstrInNode(1, "jmp", label1Node));
    EXPECT_TRUE(expectInstrInNode(2, "store", label1Node));

    EXPECT_TRUE(expectInstrInNode(0, "sub", label2Node));
    EXPECT_TRUE(expectInstrInNode(1, "xor", label2Node));
    EXPECT_TRUE(expectInstrInNode(2, "and", label2Node));

    EXPECT_TRUE(expectInstrInNode(0, "not", label3Node));
    EXPECT_TRUE(expectInstrInNode(1, "or", label3Node));
    EXPECT_TRUE(expectInstrInNode(2, "xor", label3Node));
}