/**
 * @file T_ParserBatch.cpp
 * @brief Unit tests for ParserBatch: ordering, rollback, fatal propagation.
 */
#include <gtest/gtest.h>
#include <EzLexer.h>

class ParserBatchTests : public ::testing::Test
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

    std::shared_ptr<BasicParsingContext> makeCtx(const std::string &src)
    {
        size_t id = sm->addSourceContent("test", src);
        BasicTokenizer tok(ec, sm);
        tok.tokenizeBuffer(0, id);
        ctx = std::make_shared<BasicParsingContext>(ec, sm, tok.getTokens());
        return ctx;
    }
};

TEST_F(ParserBatchTests, FirstParserMatchesReturnsIt)
{
    auto ctx = makeCtx("break;");

    ParserBatch batch;
    batch.addParser(std::make_shared<BreakParser>());
    batch.addParser(std::make_shared<ContinueParser>());

    auto result = batch.parse(ctx);
    ASSERT_NE(result.m_node, nullptr);
    EXPECT_EQ(result.m_node->getType(), AstNodeType::Break);
}

TEST_F(ParserBatchTests, FallsBackToSecondParser)
{
    auto ctx = makeCtx("continue;");

    ParserBatch batch;
    batch.addParser(std::make_shared<BreakParser>());
    batch.addParser(std::make_shared<ContinueParser>());

    auto result = batch.parse(ctx);
    ASSERT_NE(result.m_node, nullptr);
    EXPECT_EQ(result.m_node->getType(), AstNodeType::Continue);
}

TEST_F(ParserBatchTests, NoMatchReturnsNullNode)
{
    auto ctx = makeCtx("42");

    ParserBatch batch;
    batch.addParser(std::make_shared<BreakParser>());
    batch.addParser(std::make_shared<ContinueParser>());

    auto result = batch.parse(ctx);
    EXPECT_EQ(result.m_node, nullptr);
}

TEST_F(ParserBatchTests, EmptyBatchReturnsNullNode)
{
    auto ctx = makeCtx("break;");

    ParserBatch batch;
    auto result = batch.parse(ctx);
    EXPECT_EQ(result.m_node, nullptr);
}

TEST_F(ParserBatchTests, RollbackOnMissPreservesPosition)
{
    // Two tokens: "break" ";"
    auto ctx = makeCtx("break;");

    size_t posBefore = ctx->getCurrentPosition();

    ParserBatch batch;
    batch.addParser(std::make_shared<ContinueParser>()); // Will miss.

    batch.parse(ctx);

    // After a miss, position must be unchanged.
    EXPECT_EQ(ctx->getCurrentPosition(), posBefore);
}

TEST_F(ParserBatchTests, AddNullParserThrows)
{
    ParserBatch batch;
    EXPECT_THROW(batch.addParser(nullptr), std::runtime_error);
}

TEST_F(ParserBatchTests, AddParsersFromTypeList)
{
    auto ctx = makeCtx("continue;");

    ParserBatch batch;
    batch.addParsersFromTypeList<BreakParser, ContinueParser>();

    auto result = batch.parse(ctx);
    ASSERT_NE(result.m_node, nullptr);
    EXPECT_EQ(result.m_node->getType(), AstNodeType::Continue);
}

TEST_F(ParserBatchTests, MultipleParsersNoneMatchPreservesPosition)
{
    auto ctx = makeCtx("if");
    size_t posBefore = ctx->getCurrentPosition();

    ParserBatch batch;
    batch.addParser(std::make_shared<BreakParser>());
    batch.addParser(std::make_shared<ContinueParser>());

    auto result = batch.parse(ctx);
    EXPECT_EQ(result.m_node, nullptr);
    EXPECT_EQ(ctx->getCurrentPosition(), posBefore);
}
