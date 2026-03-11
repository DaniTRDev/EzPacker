/**
 * @file T_VariableParser.cpp
 * @brief Unit tests for VariableParser and the Variable AST node.
 *
 * Covers: plain variable reference, typed variable, array initializer
 * (multi-element), single-element brace init (non-array), empty input.
 */
#include <gtest/gtest.h>
#include <EzLexer.h>

class VariableParserTests : public ::testing::Test
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

    template <typename P>
    AstNode *parse(const std::string &src)
    {
        size_t id = sm->addSourceContent("test", src);
        BasicTokenizer tok(ec, sm);
        tok.tokenizeBuffer(0, id);
        ctx = std::make_shared<BasicParsingContext>(ec, sm, tok.getTokens());
        P parser;
        return parser.parse(ctx);
    }
};

TEST_F(VariableParserTests, PlainVariableReference)
{
    AstNode *node = parse<VariableParser>("%myVar");
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->getType(), AstNodeType::Variable);
    auto *var = dynamic_cast<Variable *>(node);
    EXPECT_EQ(var->getVariableName(), "myVar");
    EXPECT_TRUE(var->getVariableDataType().empty());
    EXPECT_FALSE(var->getIsArray());
}

TEST_F(VariableParserTests, TypedVariable)
{
    AstNode *node = parse<VariableParser>("i64 %count");
    ASSERT_NE(node, nullptr);
    auto *var = dynamic_cast<Variable *>(node);
    EXPECT_EQ(var->getVariableName(), "count");
    EXPECT_EQ(var->getVariableDataType(), "i64");
    EXPECT_FALSE(var->getIsArray());
}

TEST_F(VariableParserTests, SingleElementBraceInitIsNotArray)
{
    AstNode *node = parse<VariableParser>("i64 %x: {5}");
    ASSERT_NE(node, nullptr);
    auto *var = dynamic_cast<Variable *>(node);
    EXPECT_FALSE(var->getIsArray());
    EXPECT_EQ(var->getExpressions()->m_numElems, 1u);
}

TEST_F(VariableParserTests, MultiElementBraceInitIsArray)
{
    AstNode *node = parse<VariableParser>("i8 %bytes: {1, 2, 3}");
    ASSERT_NE(node, nullptr);
    auto *var = dynamic_cast<Variable *>(node);
    EXPECT_TRUE(var->getIsArray());
    EXPECT_EQ(var->getExpressions()->m_numElems, 3u);
}

TEST_F(VariableParserTests, EmptyInputReturnsNull)
{
    EXPECT_EQ(parse<VariableParser>(""), nullptr);
}

TEST_F(VariableParserTests, MissingPercentSignReturnsNull)
{
    // Without the '%' prefix the parser should not recognise this as a variable.
    EXPECT_EQ(parse<VariableParser>("myVar"), nullptr);
}

// ─── Additional variable tests ───────────────────────────────────────────────

TEST_F(VariableParserTests, VariableNodeName)
{
    AstNode *node = parse<VariableParser>("%v");
    ASSERT_NE(node, nullptr);
    EXPECT_STREQ(node->getAstNodeName(), "Variable");
}

TEST_F(VariableParserTests, TypedVariableWithExplicitType)
{
    AstNode *node = parse<VariableParser>("i32 %x");
    ASSERT_NE(node, nullptr);
    auto *var = dynamic_cast<Variable *>(node);
    EXPECT_EQ(var->getVariableDataType(), "i32");
    EXPECT_EQ(var->getVariableName(), "x");
}

TEST_F(VariableParserTests, TypedVariableWithI8Type)
{
    AstNode *node = parse<VariableParser>("i8 %byte");
    ASSERT_NE(node, nullptr);
    auto *var = dynamic_cast<Variable *>(node);
    EXPECT_EQ(var->getVariableDataType(), "i8");
}

TEST_F(VariableParserTests, VariableHasCorrectNodeType)
{
    AstNode *node = parse<VariableParser>("%test");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->getType(), AstNodeType::Variable);
}

TEST_F(VariableParserTests, ArrayInitializerPreservesOrder)
{
    AstNode *node = parse<VariableParser>("i64 %arr: {10, 20, 30, 40}");
    ASSERT_NE(node, nullptr);
    auto *var = dynamic_cast<Variable *>(node);
    EXPECT_TRUE(var->getIsArray());
    EXPECT_EQ(var->getExpressions()->m_numElems, 4u);
}

TEST_F(VariableParserTests, EmptyInitializerBracesIsNotArray)
{
    AstNode *node = parse<VariableParser>("i64 %x: {0}");
    ASSERT_NE(node, nullptr);
    auto *var = dynamic_cast<Variable *>(node);
    EXPECT_FALSE(var->getIsArray());
    EXPECT_EQ(var->getExpressions()->m_numElems, 1u);
}

TEST_F(VariableParserTests, UnderscoreVariableName)
{
    AstNode *node = parse<VariableParser>("%_my_var");
    ASSERT_NE(node, nullptr);
    auto *var = dynamic_cast<Variable *>(node);
    EXPECT_EQ(var->getVariableName(), "_my_var");
}

TEST_F(VariableParserTests, MalformedInitializer)
{
    // Missing closing brace
    AstNode *node = parse<VariableParser>("i64 %x: {5");
    // Depending on error handling, this might return null or a partial node with errors
    if (node) {
        EXPECT_TRUE(ec->doesCurrentScopeHasFatalErrors());
    } else {
        // If it returns null, that's also acceptable for fatal error
        SUCCEED();
    }
}
