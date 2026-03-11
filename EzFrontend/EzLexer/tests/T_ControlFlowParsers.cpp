/**
 * @file T_ControlFlowParsers.cpp
 * @brief Unit tests for If/While/For/Switch/Break/Continue parsers and nodes.
 */
#include <gtest/gtest.h>
#include <EzLexer.h>

class ControlFlowParserTests : public ::testing::Test
{
protected:
    size_t currentTestId = 0;
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<BasicParsingContext> ctx;

    void SetUp() override
    {
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(
            std::filesystem::current_path().empty() ? "" : std::filesystem::current_path());
        ec->beginScope();
        ctx = nullptr;
    }

    void TearDown() override
    {
        ec->endScope(ErrorAction::Discard);
    }

    template <typename P> AstNode *parse(const std::string &src)
    {
        std::string sourceName = std::format("test_{}", currentTestId++);
        size_t id = sm->addSourceContent(sourceName, src);
        BasicTokenizer tok(ec, sm);
        tok.tokenizeBuffer(0, id);
        ctx = std::make_shared<BasicParsingContext>(ec, sm, tok.getTokens());
        P parser;
        return parser.parse(ctx);
    }
};

// ─── IfParser ────────────────────────────────────────────────────────────────

TEST_F(ControlFlowParserTests, SimpleIfNoElse)
{
    AstNode *node = parse<IfParser>("if (%a EQ %b) { nop; }");
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->getType(), AstNodeType::If);
    auto *ifNode = dynamic_cast<IfAstNode *>(node);
    ASSERT_NE(ifNode->getCondition(), nullptr);
    ASSERT_NE(ifNode->getTrueScope(), nullptr);
    EXPECT_EQ(ifNode->getFalseScope(), nullptr);
}

TEST_F(ControlFlowParserTests, IfWithElse)
{
    AstNode *node = parse<IfParser>("if (%x GT 0) { nop; } else { nop; }");
    ASSERT_NE(node, nullptr);
    auto *ifNode = dynamic_cast<IfAstNode *>(node);
    ASSERT_NE(ifNode->getFalseScope(), nullptr);
    EXPECT_EQ(ifNode->getFalseScope()->getType(), AstNodeType::CodeScope);
}

TEST_F(ControlFlowParserTests, IfWithElseIf)
{
    AstNode *node = parse<IfParser>("if (%x GT 0) { nop; } else if (%x LT 0) { nop; }");
    ASSERT_NE(node, nullptr);
    auto *ifNode = dynamic_cast<IfAstNode *>(node);
    ASSERT_NE(ifNode->getFalseScope(), nullptr);
    // The else-if is itself an IfAstNode.
    EXPECT_EQ(ifNode->getFalseScope()->getType(), AstNodeType::If);
}

TEST_F(ControlFlowParserTests, AllComparisonOperators)
{
    struct TC
    {
        const char *src;
        ConditionComparisonType expected;
    };
    TC cases[] = {
        { "if (%a EQ %b) {}", ConditionComparisonType::Equal },
        { "if (%a NE %b) {}", ConditionComparisonType::NotEqual },
        { "if (%a LT %b) {}", ConditionComparisonType::LessThan },
        { "if (%a GT %b) {}", ConditionComparisonType::GreaterThan },
        { "if (%a GE %b) {}", ConditionComparisonType::GreaterThanOrEqual },
        { "if (%a LE %b) {}", ConditionComparisonType::LessThanOrEqual },
    };
    for (auto &tc : cases)
    {
        AstNode *node = parse<IfParser>(tc.src);
        ASSERT_NE(node, nullptr) << "source: " << tc.src;
        auto *ifNode = dynamic_cast<IfAstNode *>(node);
        EXPECT_EQ(ifNode->getCondition()->getComparisonType(), tc.expected) << "source: " << tc.src;
    }
}

TEST_F(ControlFlowParserTests, IfEmptyInputReturnsNull)
{
    EXPECT_EQ(parse<IfParser>(""), nullptr);
}

// ─── WhileParser ─────────────────────────────────────────────────────────────

TEST_F(ControlFlowParserTests, SimpleWhile)
{
    AstNode *node = parse<WhileParser>("while (%i LT 10) { nop; }");
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->getType(), AstNodeType::While);
    auto *w = dynamic_cast<WhileAstNode *>(node);
    ASSERT_NE(w->getCondition(), nullptr);
    ASSERT_NE(w->getCodeScope(), nullptr);
    EXPECT_GE(w->getCodeScope()->getExpressions()->m_numElems, 1u);
}

TEST_F(ControlFlowParserTests, EmptyWhileBody)
{
    AstNode *node = parse<WhileParser>("while (%x EQ 0) {}");
    ASSERT_NE(node, nullptr);
    auto *w = dynamic_cast<WhileAstNode *>(node);
    EXPECT_EQ(w->getCodeScope()->getExpressions()->m_numElems, 0u);
}

TEST_F(ControlFlowParserTests, WhileEmptyInputReturnsNull)
{
    EXPECT_EQ(parse<WhileParser>(""), nullptr);
}

// ─── ForParser ───────────────────────────────────────────────────────────────

TEST_F(ControlFlowParserTests, SimpleFor)
{
    AstNode *node = parse<ForParser>("for ({create i64 %i; mov %i, 0;} (%i LT 10) {add %i, 1;}) { nop; }");
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->getType(), AstNodeType::For);
    auto *forNode = dynamic_cast<ForAstNode *>(node);
    ASSERT_NE(forNode->getInitialization(), nullptr);
    ASSERT_NE(forNode->getCondition(), nullptr);
    ASSERT_NE(forNode->getNextItClause(), nullptr);
    ASSERT_NE(forNode->getBody(), nullptr);
}

TEST_F(ControlFlowParserTests, ForEmptyInputReturnsNull)
{
    EXPECT_EQ(parse<ForParser>(""), nullptr);
}

// ─── SwitchParser ─────────────────────────────────────────────────────────────

TEST_F(ControlFlowParserTests, SwitchWithCasesAndDefault)
{
    AstNode *node = parse<SwitchParser>("switch (%x) { "
                                          "  case 1: { nop; } "
                                          "  case 2: { nop; } "
                                          "  default: { nop; } "
                                          "}");
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->getType(), AstNodeType::Switch);
    auto *sw = dynamic_cast<SwitchAstNode *>(node);
    ASSERT_NE(sw->getSwitchVariable(), nullptr);
    EXPECT_EQ(sw->getSwitchVariable()->getVariableName(), "x");
    ASSERT_NE(sw->getCases(), nullptr);
    EXPECT_EQ(sw->getCases()->m_numElems, 2u);
    ASSERT_NE(sw->getDefault(), nullptr);
}

TEST_F(ControlFlowParserTests, SwitchNoCases)
{
    AstNode *node = parse<SwitchParser>("switch (%x) {}");
    ASSERT_NE(node, nullptr);
    auto *sw = dynamic_cast<SwitchAstNode *>(node);
    // No cases and no default.
    EXPECT_EQ(sw->getDefault(), nullptr);
}

TEST_F(ControlFlowParserTests, SwitchEmptyInputReturnsNull)
{
    EXPECT_EQ(parse<SwitchParser>(""), nullptr);
}

// ─── BreakParser ─────────────────────────────────────────────────────────────

TEST_F(ControlFlowParserTests, SimpleBreak)
{
    AstNode *node = parse<BreakParser>("break;");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->getType(), AstNodeType::Break);
}

TEST_F(ControlFlowParserTests, BreakEmptyInputReturnsNull)
{
    EXPECT_EQ(parse<BreakParser>(""), nullptr);
}

// ─── ContinueParser ──────────────────────────────────────────────────────────

TEST_F(ControlFlowParserTests, SimpleContinue)
{
    AstNode *node = parse<ContinueParser>("continue;");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->getType(), AstNodeType::Continue);
}

TEST_F(ControlFlowParserTests, ContinueEmptyInputReturnsNull)
{
    EXPECT_EQ(parse<ContinueParser>(""), nullptr);
}

// ─── Additional control-flow edge-cases ───────────────────────────────────────

TEST_F(ControlFlowParserTests, ConditionLeftAndRightAreVariables)
{
    AstNode *node = parse<IfParser>("if (%a EQ %b) { nop; }");
    ASSERT_NE(node, nullptr);
    auto *ifNode = dynamic_cast<IfAstNode *>(node);
    ASSERT_NE(ifNode->getCondition(), nullptr);
    ASSERT_NE(ifNode->getCondition()->getLeft(), nullptr);
    ASSERT_NE(ifNode->getCondition()->getRight(), nullptr);
    EXPECT_EQ(ifNode->getCondition()->getLeft()->getType(), AstNodeType::Variable);
    EXPECT_EQ(ifNode->getCondition()->getRight()->getType(), AstNodeType::Variable);
}

TEST_F(ControlFlowParserTests, ConditionRightCanBeImmediate)
{
    AstNode *node = parse<IfParser>("if (%x GT 0) { nop; }");
    ASSERT_NE(node, nullptr);
    auto *ifNode = dynamic_cast<IfAstNode *>(node);
    EXPECT_EQ(ifNode->getCondition()->getRight()->getType(), AstNodeType::Immediate);
}

TEST_F(ControlFlowParserTests, NestedIf)
{
    AstNode *node = parse<IfParser>("if (%a GT 0) { if (%b GT 0) { nop; } }");
    ASSERT_NE(node, nullptr);
    auto *ifNode = dynamic_cast<IfAstNode *>(node);
    ASSERT_NE(ifNode->getTrueScope(), nullptr);
    EXPECT_GE(ifNode->getTrueScope()->getExpressions()->m_numElems, 1u);
}

TEST_F(ControlFlowParserTests, WhileWithMultipleInstructions)
{
    AstNode *node = parse<WhileParser>("while (%i LT 10) { nop; nop; nop; }");
    ASSERT_NE(node, nullptr);
    auto *w = dynamic_cast<WhileAstNode *>(node);
    EXPECT_EQ(w->getCodeScope()->getExpressions()->m_numElems, 3u);
}

TEST_F(ControlFlowParserTests, WhileConditionNodeType)
{
    AstNode *node = parse<WhileParser>("while (%x EQ 0) { nop; }");
    ASSERT_NE(node, nullptr);
    auto *w = dynamic_cast<WhileAstNode *>(node);
    EXPECT_EQ(w->getCondition()->getType(), AstNodeType::Condition);
}

TEST_F(ControlFlowParserTests, ForBodyContainsInstructions)
{
    AstNode *node = parse<ForParser>("for ({create i64 %i; mov %i, 0;} (%i LT 10) {add %i, 1;}) { nop; nop; }");
    ASSERT_NE(node, nullptr);
    auto *forNode = dynamic_cast<ForAstNode *>(node);
    EXPECT_EQ(forNode->getBody()->getExpressions()->m_numElems, 2u);
}

TEST_F(ControlFlowParserTests, SwitchWithOnlyDefault)
{
    AstNode *node = parse<SwitchParser>("switch (%x) { "
                                          "  default: { nop; } "
                                          "}");
    ASSERT_NE(node, nullptr);
    auto *sw = dynamic_cast<SwitchAstNode *>(node);
    ASSERT_NE(sw->getDefault(), nullptr);
}

TEST_F(ControlFlowParserTests, SwitchCaseBodyHasInstructions)
{
    AstNode *node = parse<SwitchParser>("switch (%x) { "
                                          "  case 1: { nop; nop; } "
                                          "}");
    ASSERT_NE(node, nullptr);
    auto *sw = dynamic_cast<SwitchAstNode *>(node);
    ASSERT_NE(sw->getCases(), nullptr);
    EXPECT_EQ(sw->getCases()->m_numElems, 1u);
}

TEST_F(ControlFlowParserTests, BreakNodeName)
{
    AstNode *node = parse<BreakParser>("break;");
    ASSERT_NE(node, nullptr);
    EXPECT_STREQ(node->getAstNodeName(), "BreakAstNode");
}

TEST_F(ControlFlowParserTests, ContinueNodeName)
{
    AstNode *node = parse<ContinueParser>("continue;");
    ASSERT_NE(node, nullptr);
    EXPECT_STREQ(node->getAstNodeName(), "ContinueAstNode");
}

TEST_F(ControlFlowParserTests, IfNodeName)
{
    AstNode *node = parse<IfParser>("if (%a EQ %b) {}");
    ASSERT_NE(node, nullptr);
    EXPECT_STREQ(node->getAstNodeName(), "IfAstNode");
}

TEST_F(ControlFlowParserTests, WhileNodeName)
{
    AstNode *node = parse<WhileParser>("while (%x EQ 0) {}");
    ASSERT_NE(node, nullptr);
    EXPECT_STREQ(node->getAstNodeName(), "WhileAstNode");
}

TEST_F(ControlFlowParserTests, ForNodeName)
{
    AstNode *node = parse<ForParser>("for ({create i64 %i: {0};} (%i LT 10) {add %i, 1;}) {}");
    ASSERT_NE(node, nullptr);
    EXPECT_STREQ(node->getAstNodeName(), "ForAstNode");
}

TEST_F(ControlFlowParserTests, SwitchNodeName)
{
    AstNode *node = parse<SwitchParser>("switch (%x) {}");
    ASSERT_NE(node, nullptr);
    EXPECT_STREQ(node->getAstNodeName(), "SwitchAstNode");
}

TEST_F(ControlFlowParserTests, IfMissingCondition)
{
    AstNode *node = parse<IfParser>("if () {}");
    if (node != nullptr)
        EXPECT_TRUE(ec->doesCurrentScopeHasFatalErrors());
    else
        SUCCEED();
}
