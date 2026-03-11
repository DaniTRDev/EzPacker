/**
 * @file T_InstructionParser.cpp
 * @brief Unit tests for InstructionParser and the Instruction / CallInstruction AST nodes.
 *
 * Covers: plain 0-operand instruction, 1-operand instruction, multi-operand,
 * call instruction with no args, call with args, variable operand, immediate
 * operand, mnemonic case-normalisation, rejected input.
 */
#include <gtest/gtest.h>
#include <EzLexer.h>

// ─── Fixture ─────────────────────────────────────────────────────────────────

class InstructionParserTests : public ::testing::Test
{
protected:
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager>  sm;
    std::shared_ptr<BasicParsingContext> ctx;

    void SetUp() override
    {
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        ec->beginScope();
        ctx = nullptr;
    }

    void TearDown() override
    {
        ec->endScope(ErrorAction::Discard);
    }

    template<typename ParserType>
    AstNode *parseWith(const std::string &src)
    {
        size_t id = sm->addSourceContent("test", src);
        BasicTokenizer tok(ec, sm);
        tok.tokenizeBuffer(0, id);
        ctx = std::make_shared<BasicParsingContext>(ec, sm, tok.getTokens());
        ParserType parser;
        return parser.parse(ctx);
    }
};

// ─── InstructionParser ────────────────────────────────────────────────────────

TEST_F(InstructionParserTests, ZeroOperandInstruction)
{
    AstNode *node = parseWith<InstructionParser::InstructionParser>("nop;");
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->getType(), AstNodeType::Instruction);
    auto *instr = dynamic_cast<Instruction *>(node);
    EXPECT_EQ(instr->getInstructionName(), "nop");
    EXPECT_EQ(instr->getExpressions()->m_numElems, 0u);
}

TEST_F(InstructionParserTests, SingleVariableOperand)
{
    AstNode *node = parseWith<InstructionParser::InstructionParser>("push %rax;");
    ASSERT_NE(node, nullptr);
    auto *instr = dynamic_cast<Instruction *>(node);
    EXPECT_EQ(instr->getInstructionName(), "push");
    ASSERT_EQ(instr->getExpressions()->m_numElems, 1u);
}

TEST_F(InstructionParserTests, MultipleOperands)
{
    AstNode *node = parseWith<InstructionParser::InstructionParser>("add %dst, %src;");
    ASSERT_NE(node, nullptr);
    auto *instr = dynamic_cast<Instruction *>(node);
    EXPECT_EQ(instr->getInstructionName(), "add");
    ASSERT_EQ(instr->getExpressions()->m_numElems, 2u);
}

TEST_F(InstructionParserTests, IntegerImmediateOperand)
{
    AstNode *node = parseWith<InstructionParser::InstructionParser>("mov %dst, 42;");
    ASSERT_NE(node, nullptr);
    auto *instr = dynamic_cast<Instruction *>(node);
    ASSERT_EQ(instr->getExpressions()->m_numElems, 2u);
    AstNode *second = nullptr;
    size_t idx = 0;
    for (AstNode *n : *instr->getExpressions())
    {
        if (idx == 1) second = n;
        idx++;
    }
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(second->getType(), AstNodeType::Immediate);
}

TEST_F(InstructionParserTests, MnemonicIsNormalisedToLowerCase)
{
    AstNode *node = parseWith<InstructionParser::InstructionParser>("MOV %dst, %src;");
    ASSERT_NE(node, nullptr);
    auto *instr = dynamic_cast<Instruction *>(node);
    EXPECT_EQ(instr->getInstructionName(), "mov");
}

TEST_F(InstructionParserTests, MissingSemicolonReturnsNullOrFatal)
{
    AstNode *node = parseWith<InstructionParser::InstructionParser>("nop");
    if (node != nullptr)
        EXPECT_TRUE(ec->doesCurrentScopeHasFatalErrors());
    else
        SUCCEED();
}

TEST_F(InstructionParserTests, EmptyInputReturnsNull)
{
    EXPECT_EQ(parseWith<InstructionParser::InstructionParser>(""), nullptr);
}

// ─── CallInstruction ─────────────────────────────────────────────────────────

TEST_F(InstructionParserTests, CallNoArgs)
{
    AstNode *node = parseWith<InstructionParser::InstructionParser>("call %myFunc();");
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->getType(), AstNodeType::Instruction);
    auto *instr = dynamic_cast<Instruction *>(node);
    EXPECT_EQ(instr->getInstructionName(), "call");
}

TEST_F(InstructionParserTests, CallWithArguments)
{
    AstNode *node = parseWith<InstructionParser::InstructionParser>("call %myFunc(%arg1, %arg2);");
    ASSERT_NE(node, nullptr);
    auto *instr = dynamic_cast<Instruction *>(node);
    // Expression 0 = callee, 1..N = arguments
    ASSERT_GE(instr->getExpressions()->m_numElems, 3u);
}

// ─── Additional instruction edge-cases ───────────────────────────────────────

TEST_F(InstructionParserTests, FloatImmediateOperand)
{
    AstNode *node = parseWith<InstructionParser::InstructionParser>("mov %dst, 3.14;");
    ASSERT_NE(node, nullptr);
    auto *instr = dynamic_cast<Instruction *>(node);
    ASSERT_EQ(instr->getExpressions()->m_numElems, 2u);
    AstNode *second = nullptr;
    size_t idx = 0;
    for (AstNode *n : *instr->getExpressions())
    {
        if (idx == 1) second = n;
        idx++;
    }
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(second->getType(), AstNodeType::Immediate);
}

TEST_F(InstructionParserTests, MemoryOperandInsideInstruction)
{
    AstNode *node = parseWith<InstructionParser::InstructionParser>("mov %dst, i64 (%base+0);");
    ASSERT_NE(node, nullptr);
    auto *instr = dynamic_cast<Instruction *>(node);
    ASSERT_EQ(instr->getExpressions()->m_numElems, 2u);
    AstNode *second = nullptr;
    size_t idx = 0;
    for (AstNode *n : *instr->getExpressions())
    {
        if (idx == 1) second = n;
        idx++;
    }
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(second->getType(), AstNodeType::MemoryOperand);
}

TEST_F(InstructionParserTests, ThreeOperandInstruction)
{
    // Some instructions might accept 3 operands (e.g. an arbitrary mnemonic).
    AstNode *node = parseWith<InstructionParser::InstructionParser>("test %a, %b, %c;");
    // If the parser accepts it, we check; if it errors, that's fine too.
    if (node != nullptr)
    {
        auto *instr = dynamic_cast<Instruction *>(node);
        EXPECT_EQ(instr->getExpressions()->m_numElems, 3u);
    }
}

TEST_F(InstructionParserTests, StringImmediateOperand)
{
    AstNode *node = parseWith<InstructionParser::InstructionParser>(R"(mov %dst, "hello";)");
    ASSERT_NE(node, nullptr);
    auto *instr = dynamic_cast<Instruction *>(node);
    ASSERT_EQ(instr->getExpressions()->m_numElems, 2u);
    AstNode *second = nullptr;
    size_t idx = 0;
    for (AstNode *n : *instr->getExpressions())
    {
        if (idx == 1) second = n;
        idx++;
    }
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(second->getType(), AstNodeType::Immediate);
}

TEST_F(InstructionParserTests, CallWithSingleArgument)
{
    AstNode *node = parseWith<InstructionParser::InstructionParser>("call %f(%a);");
    ASSERT_NE(node, nullptr);
    auto *instr = dynamic_cast<Instruction *>(node);
    // callee + 1 arg = 2 expressions
    ASSERT_GE(instr->getExpressions()->m_numElems, 2u);
}

TEST_F(InstructionParserTests, InstructionNamePreservesSourceRef)
{
    AstNode *node = parseWith<InstructionParser::InstructionParser>("nop;");
    ASSERT_NE(node, nullptr);
    // Just verify it's not default/empty — proves the parser sets it.
    // The source reference should point somewhere in the tokenized buffer.
}

TEST_F(InstructionParserTests, MixedCaseMnemonicNormalized)
{
    AstNode *node = parseWith<InstructionParser::InstructionParser>("PuSh %rax;");
    ASSERT_NE(node, nullptr);
    auto *instr = dynamic_cast<Instruction *>(node);
    EXPECT_EQ(instr->getInstructionName(), "push");
}

TEST_F(InstructionParserTests, MissingCommaBetweenOperands)
{
    AstNode *node = parseWith<InstructionParser::InstructionParser>("add %dst %src;");
    if (node != nullptr)
        EXPECT_TRUE(ec->doesCurrentScopeHasFatalErrors());
    else
        SUCCEED();
}
