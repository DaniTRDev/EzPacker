/**
 * @file T_MemoryParser.cpp
 * @brief Unit tests for MemoryOperandParser and all MemoryOperandAstNode sub-types.
 *
 * Covers: BaseDisplacement (+ and -), BaseIndexScaleDisplacement, IndexScale,
 * Direct, missing type prefix, empty input.
 */
#include <gtest/gtest.h>
#include <EzLexer.h>

class MemoryParserTests : public ::testing::Test
{
  protected:
    size_t currentTestId;
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<BasicParsingContext> ctx;

    void SetUp() override
    {
        currentTestId = 0;
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        ec->beginScope();
        ctx = nullptr;
    }

    void TearDown() override { ec->endScope(ErrorAction::Discard); }

    template <typename P> AstNode *parse(const std::string &src)
    {
        std::string srcName = std::format("test_{}", currentTestId++);
        size_t id = sm->addSourceContent(srcName, src);
        BasicTokenizer tok(ec, sm);
        tok.tokenizeBuffer(0, id);
        ctx = std::make_shared<BasicParsingContext>(ec, sm, tok.getTokens());
        P parser;
        return parser.parse(ctx);
    }
};

TEST_F(MemoryParserTests, BaseDisplacementPositive)
{
    AstNode *node = parse<MemoryOperandParser::MemoryOperandParser>("i64 (%base+8)");
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->getType(), AstNodeType::MemoryOperand);
    auto *mem = dynamic_cast<MemoryOperandAstNode *>(node);
    EXPECT_EQ(mem->getMemoryOperandType(), MemoryOperandType::BaseDisplacement);
    EXPECT_EQ(mem->getReferencedMemoryDataTypeStr(), "i64");
    auto *bd = dynamic_cast<BaseDisplacementMemory *>(mem);
    ASSERT_NE(bd->getBase(), nullptr);
    EXPECT_EQ(bd->getBase()->getVariableName(), "base");
    ASSERT_NE(bd->getDisplacement(), nullptr);
}

TEST_F(MemoryParserTests, BaseDisplacementNegative)
{
    AstNode *node = parse<MemoryOperandParser::MemoryOperandParser>("i32 (%base-4)");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(dynamic_cast<MemoryOperandAstNode *>(node)->getMemoryOperandType(), MemoryOperandType::BaseDisplacement);
}

TEST_F(MemoryParserTests, BaseDisplacementZero)
{
    AstNode *node = parse<MemoryOperandParser::MemoryOperandParser>("i8 (%base+0)");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(dynamic_cast<MemoryOperandAstNode *>(node)->getMemoryOperandType(), MemoryOperandType::BaseDisplacement);
}

TEST_F(MemoryParserTests, BaseIndexScaleDisplacement)
{
    AstNode *node = parse<MemoryOperandParser::MemoryOperandParser>("i64 (%base, %index, 4, 0)");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(dynamic_cast<MemoryOperandAstNode *>(node)->getMemoryOperandType(),
              MemoryOperandType::BaseIndexScaleDisplacement);
}

TEST_F(MemoryParserTests, IndexScale)
{
    AstNode *node = parse<MemoryOperandParser::MemoryOperandParser>("i64 (, %index, 8)");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(dynamic_cast<MemoryOperandAstNode *>(node)->getMemoryOperandType(), MemoryOperandType::IndexScale);
}

TEST_F(MemoryParserTests, DirectAddress)
{
    AstNode *node = parse<MemoryOperandParser::MemoryOperandParser>("i32 (0xDEADBEEF)");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(dynamic_cast<MemoryOperandAstNode *>(node)->getMemoryOperandType(), MemoryOperandType::Direct);
}

TEST_F(MemoryParserTests, EmptyInputReturnsNull)
{
    EXPECT_EQ(parse<MemoryOperandParser::MemoryOperandParser>(""), nullptr);
}

// ─── Additional memory operand tests ──────────────────────────────────────────

TEST_F(MemoryParserTests, BaseDisplacementNodeName)
{
    AstNode *node = parse<MemoryOperandParser::MemoryOperandParser>("i64 (%base+8)");
    ASSERT_NE(node, nullptr);
    EXPECT_STREQ(node->getAstNodeName(), "MemoryOperand");
}

TEST_F(MemoryParserTests, BaseDisplacementTypeNameBD)
{
    AstNode *node = parse<MemoryOperandParser::MemoryOperandParser>("i64 (%base+8)");
    ASSERT_NE(node, nullptr);
    auto *mem = dynamic_cast<MemoryOperandAstNode *>(node);
    EXPECT_STREQ(mem->getMemoryOperandTypeName(), "BaseDisplacement");
}

TEST_F(MemoryParserTests, IndexScaleTypeName)
{
    AstNode *node = parse<MemoryOperandParser::MemoryOperandParser>("i64 (, %index, 8)");
    ASSERT_NE(node, nullptr);
    auto *mem = dynamic_cast<MemoryOperandAstNode *>(node);
    EXPECT_STREQ(mem->getMemoryOperandTypeName(), "IndexScale");
}

TEST_F(MemoryParserTests, DirectTypeNameStr)
{
    AstNode *node = parse<MemoryOperandParser::MemoryOperandParser>("i32 (0xDEADBEEF)");
    ASSERT_NE(node, nullptr);
    auto *mem = dynamic_cast<MemoryOperandAstNode *>(node);
    EXPECT_STREQ(mem->getMemoryOperandTypeName(), "Direct");
}

TEST_F(MemoryParserTests, BaseIndexScaleDisplacementTypeName)
{
    AstNode *node = parse<MemoryOperandParser::MemoryOperandParser>("i64 (%base, %index, 4, 0)");
    ASSERT_NE(node, nullptr);
    auto *mem = dynamic_cast<MemoryOperandAstNode *>(node);
    EXPECT_STREQ(mem->getMemoryOperandTypeName(), "BaseIndexScaleDisplacement");
}

TEST_F(MemoryParserTests, BaseDisplacementWithDifferentTypes)
{
    // Test with i8
    AstNode *node8 = parse<MemoryOperandParser::MemoryOperandParser>("i8 (%base+1)");
    ASSERT_NE(node8, nullptr);
    EXPECT_EQ(dynamic_cast<MemoryOperandAstNode *>(node8)->getReferencedMemoryDataTypeStr(), "i8");

    // Test with i16
    AstNode *node16 = parse<MemoryOperandParser::MemoryOperandParser>("i16 (%base+2)");
    ASSERT_NE(node16, nullptr);
    EXPECT_EQ(dynamic_cast<MemoryOperandAstNode *>(node16)->getReferencedMemoryDataTypeStr(), "i16");

    // Test with i32
    AstNode *node32 = parse<MemoryOperandParser::MemoryOperandParser>("i32 (%base+4)");
    ASSERT_NE(node32, nullptr);
    EXPECT_EQ(dynamic_cast<MemoryOperandAstNode *>(node32)->getReferencedMemoryDataTypeStr(), "i32");
}

TEST_F(MemoryParserTests, BaseDisplacementBaseVariableName)
{
    AstNode *node = parse<MemoryOperandParser::MemoryOperandParser>("i64 (%myPtr+16)");
    ASSERT_NE(node, nullptr);
    auto *mem = dynamic_cast<MemoryOperandAstNode *>(node);
    ASSERT_NE(mem, nullptr);
    auto *bd = dynamic_cast<BaseDisplacementMemory *>(mem);
    ASSERT_NE(bd->getBase(), nullptr);
    EXPECT_EQ(bd->getBase()->getVariableName(), "myPtr");
}

TEST_F(MemoryParserTests, MemoryOperandNodeType)
{
    AstNode *node = parse<MemoryOperandParser::MemoryOperandParser>("i64 (%base+0)");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->getType(), AstNodeType::MemoryOperand);
}

TEST_F(MemoryParserTests, InvalidMemoryOperandSyntax)
{
    // Missing closing parenthesis
    EXPECT_EQ(parse<MemoryOperandParser::MemoryOperandParser>("i64 (%base+8"), nullptr);

    // Missing variable.
    EXPECT_EQ(parse<MemoryOperandParser::MemoryOperandParser>("(base+8)"), nullptr);
}
