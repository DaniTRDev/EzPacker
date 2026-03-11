/**
 * @file T_ModuleParser.cpp
 * @brief Unit tests for ModuleParser, ModuleHeader, Module AST nodes.
 *
 * Covers: empty module (no params), module with params, return type, nested
 * label, missing body brace, missing name.
 */
#include <gtest/gtest.h>
#include <EzLexer.h>

using ModParser = ModuleParser::ModuleParser;

class ModuleParserTests : public ::testing::Test
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

    std::shared_ptr<BasicParsingContext> makeCtx(const std::string &source)
    {
        size_t id = sm->addSourceContent("test", source);
        BasicTokenizer tok(ec, sm);
        tok.tokenizeBuffer(0, id);
        ctx = std::make_shared<BasicParsingContext>(ec, sm, tok.getTokens());
        return ctx;
    }
};

// ─── Module with no parameters ────────────────────────────────────────────────

TEST_F(ModuleParserTests, EmptyModuleNoParams)
{
    auto ctx = makeCtx("void myFunc() {}");
    ModuleParser::ModuleParser parser;
    AstNode *node = parser.parse(ctx);

    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->getType(), AstNodeType::Module);
    auto *mod = dynamic_cast<Module *>(node);
    ASSERT_NE(mod->getHeader(), nullptr);
    EXPECT_EQ(mod->getHeader()->getModuleName(), "myFunc");
    EXPECT_EQ(mod->getHeader()->getReturnTypeName(), "void");
    ASSERT_NE(mod->getBody(), nullptr);
    EXPECT_EQ(mod->getBody()->getExpressions()->m_numElems, 0u);
}

// ─── Module with parameters ───────────────────────────────────────────────────

TEST_F(ModuleParserTests, ModuleWithParameters)
{
    auto ctx = makeCtx("i64 add(i64 %a, i64 %b) {}");
    ModuleParser::ModuleParser parser;
    AstNode *node = parser.parse(ctx);

    ASSERT_NE(node, nullptr);
    auto *mod = dynamic_cast<Module *>(node);
    EXPECT_EQ(mod->getHeader()->getModuleName(), "add");
    EXPECT_EQ(mod->getHeader()->getReturnTypeName(), "i64");
    EXPECT_EQ(mod->getHeader()->getExpressions()->m_numElems, 2u);
}

// ─── Module with body ────────────────────────────────────────────────────────

TEST_F(ModuleParserTests, ModuleWithBodyInstruction)
{
    auto ctx = makeCtx("void foo() { nop; }");
    ModuleParser::ModuleParser parser;
    AstNode *node = parser.parse(ctx);

    ASSERT_NE(node, nullptr);
    auto *mod = dynamic_cast<Module *>(node);
    EXPECT_EQ(mod->getBody()->getExpressions()->m_numElems, 1u);
}

// ─── Module with a label in body ─────────────────────────────────────────────

TEST_F(ModuleParserTests, ModuleWithLabelInBody)
{
    auto ctx = makeCtx("void foo() { entry: { nop; } }");
    ModuleParser::ModuleParser parser;
    AstNode *node = parser.parse(ctx);

    ASSERT_NE(node, nullptr);
    auto *mod = dynamic_cast<Module *>(node);
    ASSERT_GE(mod->getBody()->getExpressions()->m_numElems, 1u);
    AstNode *first = nullptr;
    for (AstNode *n : *mod->getBody()->getExpressions()) { first = n; break; }
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->getType(), AstNodeType::Label);
}

// ─── Error cases ──────────────────────────────────────────────────────────────

TEST_F(ModuleParserTests, MissingOpenBraceReturnNullOrError)
{
    auto ctx = makeCtx("void foo()");
    ModuleParser::ModuleParser parser;
    AstNode *node = parser.parse(ctx);
    if (node != nullptr)
        EXPECT_TRUE(ec->doesCurrentScopeHasFatalErrors());
    else
        SUCCEED();
}

TEST_F(ModuleParserTests, EmptyInputReturnsNull)
{
    auto ctx = makeCtx("");
    ModuleParser::ModuleParser parser;
    EXPECT_EQ(parser.parse(ctx), nullptr);
}

// ─── Additional module tests ──────────────────────────────────────────────────

TEST_F(ModuleParserTests, ModuleWithMultipleInstructions)
{
    auto ctx = makeCtx("void foo() { nop; nop; nop; }");
    ModuleParser::ModuleParser parser;
    AstNode *node = parser.parse(ctx);
    ASSERT_NE(node, nullptr);
    auto *mod = dynamic_cast<Module *>(node);
    EXPECT_EQ(mod->getBody()->getExpressions()->m_numElems, 3u);
}

TEST_F(ModuleParserTests, ModuleWithSingleParameter)
{
    auto ctx = makeCtx("i32 square(i32 %x) {}");
    ModuleParser::ModuleParser parser;
    AstNode *node = parser.parse(ctx);
    ASSERT_NE(node, nullptr);
    auto *mod = dynamic_cast<Module *>(node);
    EXPECT_EQ(mod->getHeader()->getModuleName(), "square");
    EXPECT_EQ(mod->getHeader()->getReturnTypeName(), "i32");
    EXPECT_EQ(mod->getHeader()->getExpressions()->m_numElems, 1u);
}

TEST_F(ModuleParserTests, ModuleHeaderHasCorrectNodeType)
{
    auto ctx = makeCtx("void bar() {}");
    ModuleParser::ModuleParser parser;
    AstNode *node = parser.parse(ctx);
    ASSERT_NE(node, nullptr);
    auto *mod = dynamic_cast<Module *>(node);
    EXPECT_EQ(mod->getHeader()->getType(), AstNodeType::ModuleHeader);
}

TEST_F(ModuleParserTests, ModuleBodyNodeType)
{
    auto ctx = makeCtx("void baz() {}");
    ModuleParser::ModuleParser parser;
    AstNode *node = parser.parse(ctx);
    ASSERT_NE(node, nullptr);
    auto *mod = dynamic_cast<Module *>(node);
    EXPECT_EQ(mod->getBody()->getType(), AstNodeType::CodeScope);
}

TEST_F(ModuleParserTests, ModuleWithMultipleLabels)
{
    auto ctx = makeCtx("void foo() { a: { nop; } b: { nop; } }");
    ModuleParser::ModuleParser parser;
    AstNode *node = parser.parse(ctx);
    ASSERT_NE(node, nullptr);
    auto *mod = dynamic_cast<Module *>(node);
    EXPECT_EQ(mod->getBody()->getExpressions()->m_numElems, 2u);
}

TEST_F(ModuleParserTests, ModuleWithNestedControlFlow)
{
    auto ctx = makeCtx("void foo(i64 %x) { if (%x GT 0) { nop; } }");
    ModuleParser::ModuleParser parser;
    AstNode *node = parser.parse(ctx);
    ASSERT_NE(node, nullptr);
    auto *mod = dynamic_cast<Module *>(node);
    EXPECT_GE(mod->getBody()->getExpressions()->m_numElems, 1u);
    AstNode *first = nullptr;
    for (AstNode *n : *mod->getBody()->getExpressions()) { first = n; break; }
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->getType(), AstNodeType::If);
}

TEST_F(ModuleParserTests, MissingClosingBrace)
{
    auto ctx = makeCtx("void foo() {");
    ModuleParser::ModuleParser parser;
    AstNode *node = parser.parse(ctx);
    if (node != nullptr)
        EXPECT_TRUE(ec->doesCurrentScopeHasFatalErrors());
    else
        SUCCEED();
}
