#include "BasicAnnotatorTest.h"

TEST_F(BasicAnnotatorTest, TestInstructionNonLoadVV)
{
    std::string input = R"(
        .add .i64 %rcx, .i64 %rdx;
    )";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Instruction(), input);
    std::shared_ptr<ScopedSymbol> rcx = std::make_shared<ScopedSymbol>(m_i64TypeId, "rcx");
    std::shared_ptr<ScopedSymbol> rdx = std::make_shared<ScopedSymbol>(m_i64TypeId, "rdx");
    std::shared_ptr<InstructionAnnotator> iiAnotator =
            std::make_shared<InstructionAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    size_t rcxId = m_symbolTable->addItem(rcx); // Add the symbol.
    size_t rdxId = m_symbolTable->addItem(rdx); // Add the symbol.
    std::shared_ptr<AstNode> instrNode = node->getChild(0);

    EXPECT_TRUE(iiAnotator->annotate(instrNode, m_logger));
    EXPECT_TRUE(expectInstructionId(g_InstructionTable.at("add"), instrNode));
    EXPECT_TRUE(expectInstructionOperandVV(rcxId, m_i64TypeId, 0, instrNode));
    EXPECT_TRUE(expectInstructionOperandVV(rdxId, m_i64TypeId, 1, instrNode));
}

TEST_F(BasicAnnotatorTest, TestInstructionNonLoadVVCC)
{
    std::string input = R"(
        .add .i64 %rcx, .i64 123456;
    )";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Instruction(), input);
    std::shared_ptr<ScopedSymbol> rcx = std::make_shared<ScopedSymbol>(m_i64TypeId, "rcx");
    std::shared_ptr<InstructionAnnotator> iiAnotator =
            std::make_shared<InstructionAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    size_t rcxId = m_symbolTable->addItem(rcx); // Add the symbol.
    std::shared_ptr<AstNode> instrNode = node->getChild(0);

    EXPECT_TRUE(iiAnotator->annotate(instrNode, m_logger));
    EXPECT_TRUE(expectInstructionId(g_InstructionTable.at("add"), instrNode));
    EXPECT_TRUE(expectInstructionOperandVV(rcxId, m_i64TypeId, 0, instrNode));
    EXPECT_TRUE(expectInstructionOperandCC(123456, 1, instrNode));
}

TEST_F(BasicAnnotatorTest, TestInstructionNonLoadVVMM)
{
    std::string input = R"(
        .add .i64 %rcx, .i64 (%rcx, %rdx, 8, 1000);
    )";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Instruction(), input);
    std::shared_ptr<ScopedSymbol> rcx = std::make_shared<ScopedSymbol>(m_i64TypeId, "rcx");
    std::shared_ptr<ScopedSymbol> rdx = std::make_shared<ScopedSymbol>(m_i64TypeId, "rdx");
    std::shared_ptr<InstructionAnnotator> iiAnotator =
            std::make_shared<InstructionAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    size_t rcxId = m_symbolTable->addItem(rcx); // Add the symbol.
    size_t rdxId = m_symbolTable->addItem(rdx); // Add the symbol.
    std::shared_ptr<AstNode> instrNode = node->getChild(0);

    EXPECT_TRUE(iiAnotator->annotate(instrNode, m_logger));
    EXPECT_TRUE(expectInstructionId(g_InstructionTable.at("add"), instrNode));
    EXPECT_TRUE(expectInstructionOperandVV(rcxId, m_i64TypeId, 0, instrNode));
    EXPECT_TRUE(expectInstructionOperandMM(MemoryReferenceType::BaseIndexScaleDispl, 1, instrNode));
}

TEST_F(BasicAnnotatorTest, TestInstructionLoadVV)
{
    std::string input = R"(
        .load .i64 %rcx, .i64 %rdx;
    )";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Instruction(), input);
    std::shared_ptr<ScopedSymbol> rdx = std::make_shared<ScopedSymbol>(m_i64TypeId, "rdx");
    std::shared_ptr<InstructionAnnotator> iiAnotator =
            std::make_shared<InstructionAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    size_t rcxId = 0, rdxId = m_symbolTable->addItem(rdx); // Add the symbol.
    std::shared_ptr<AstNode> instrNode = node->getChild(0);

    EXPECT_TRUE(iiAnotator->annotate(instrNode, m_logger));
    EXPECT_TRUE(m_symbolTable->doesElemExistByString(ScopedSymbol::getStringFromName("rcx"), rcxId));
    EXPECT_EQ(m_symbolTable->getItem<ScopedSymbol>(rcxId)->getTypeId(), m_i64TypeId);
    EXPECT_TRUE(expectInstructionId(g_InstructionTable.at("load"), instrNode));
    EXPECT_TRUE(expectInstructionOperandVV(rcxId, m_i64TypeId, 0, instrNode));
    EXPECT_TRUE(expectInstructionOperandVV(rdxId, m_i64TypeId, 1, instrNode));
}

TEST_F(BasicAnnotatorTest, TestInstructionLoadVVCC)
{
    std::string input = R"(
        .load .i64 %rcx, .i64 123456;
    )";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Instruction(), input);
    std::shared_ptr<InstructionAnnotator> iiAnotator =
            std::make_shared<InstructionAnnotator>(m_symbolTable, m_typeTable);
    EXPECT_NE(node, nullptr);

    size_t rcxId = 0;
    std::shared_ptr<AstNode> instrNode = node->getChild(0);

    EXPECT_TRUE(iiAnotator->annotate(instrNode, m_logger));
    EXPECT_TRUE(m_symbolTable->doesElemExistByString(ScopedSymbol::getStringFromName("rcx"), rcxId));
    EXPECT_EQ(m_symbolTable->getItem<ScopedSymbol>(rcxId)->getTypeId(), m_i64TypeId);
    EXPECT_TRUE(expectInstructionId(g_InstructionTable.at("load"), instrNode));
    EXPECT_TRUE(expectInstructionOperandVV(rcxId, m_i64TypeId, 0, instrNode));
    EXPECT_TRUE(expectInstructionOperandCC(123456, 1, instrNode));
}

TEST_F(BasicAnnotatorTest, TestInstructionLoadVMM)
{
    std::string input = R"(
        .load .i64 %rcx, .i64 (%rcx, %rdx, 8, 1000);
    )";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Instruction(), input);
    std::shared_ptr<InstructionAnnotator> iiAnotator =
            std::make_shared<InstructionAnnotator>(m_symbolTable, m_typeTable);
    std::shared_ptr<ScopedSymbol> rdx = std::make_shared<ScopedSymbol>(m_i64TypeId, "rdx");
    EXPECT_NE(node, nullptr);

    size_t rcxId = 0, rdxId = m_symbolTable->addItem(rdx); // Add the symbol.
    std::shared_ptr<AstNode> instrNode = node->getChild(0);

    EXPECT_TRUE(iiAnotator->annotate(instrNode, m_logger));
    EXPECT_TRUE(m_symbolTable->doesElemExistByString(ScopedSymbol::getStringFromName("rcx"), rcxId));
    EXPECT_EQ(m_symbolTable->getItem<ScopedSymbol>(rcxId)->getTypeId(), m_i64TypeId);
    EXPECT_TRUE(expectInstructionId(g_InstructionTable.at("load"), instrNode));
    EXPECT_TRUE(expectInstructionOperandVV(rcxId, m_i64TypeId, 0, instrNode));
    EXPECT_TRUE(expectInstructionOperandMM(MemoryReferenceType::BaseIndexScaleDispl, 1, instrNode));
}

TEST_F(BasicAnnotatorTest, TestInstructionInvalidName)
{
    std::string input = R"(
        .asd .i64 %rcx, .i64 (%rcx, %rdx, 8, 1000);
    )";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Instruction(), input);
    std::shared_ptr<InstructionAnnotator> iiAnotator =
            std::make_shared<InstructionAnnotator>(m_symbolTable, m_typeTable);
    std::shared_ptr<ScopedSymbol> rdx = std::make_shared<ScopedSymbol>(m_i64TypeId, "rdx");
    EXPECT_NE(node, nullptr);

    size_t rcxId = 0, rdxId = m_symbolTable->addItem(rdx); // Add the symbol.
    std::shared_ptr<AstNode> instrNode = node->getChild(0);

    EXPECT_FALSE(iiAnotator->annotate(instrNode, m_logger));
}

TEST_F(BasicAnnotatorTest, TestInstructionInvalidArgType)
{
    std::string input = R"(
        .asd .invalid %rcx, .i64 (%rcx, %rdx, 8, 1000);
    )";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Instruction(), input);
    std::shared_ptr<InstructionAnnotator> iiAnotator =
            std::make_shared<InstructionAnnotator>(m_symbolTable, m_typeTable);
    std::shared_ptr<ScopedSymbol> rdx = std::make_shared<ScopedSymbol>(m_i64TypeId, "rdx");
    EXPECT_NE(node, nullptr);

    size_t rcxId = 0, rdxId = m_symbolTable->addItem(rdx); // Add the symbol.
    std::shared_ptr<AstNode> instrNode = node->getChild(0);

    EXPECT_FALSE(iiAnotator->annotate(instrNode, m_logger));
}

TEST_F(BasicAnnotatorTest, TestInstructionInvalidArgSymbol)
{
    std::string input = R"(
        .asd .i32 %invalid, .i64 (%rcx, %rdx, 8, 1000);
    )";

    std::shared_ptr<AstNode> node = parse(NodeParsers::Instruction(), input);
    std::shared_ptr<InstructionAnnotator> iiAnotator =
            std::make_shared<InstructionAnnotator>(m_symbolTable, m_typeTable);
    std::shared_ptr<ScopedSymbol> rdx = std::make_shared<ScopedSymbol>(m_i64TypeId, "rdx");
    EXPECT_NE(node, nullptr);

    size_t rcxId = 0, rdxId = m_symbolTable->addItem(rdx); // Add the symbol.
    std::shared_ptr<AstNode> instrNode = node->getChild(0);

    EXPECT_FALSE(iiAnotator->annotate(instrNode, m_logger));
}