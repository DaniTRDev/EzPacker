#include "BasicParserTest.h"

void BasicParserTest::SetUp() {}

void BasicParserTest::TearDown() { Test::TearDown(); }

void BasicParserTest::expectChildCount(size_t count, const std::shared_ptr<AstNode> &parent)
{
    ASSERT_NE(parent, nullptr);
    ASSERT_FALSE(parent->getChildren().size() < count);
}

void BasicParserTest::expectChildNodeType(size_t type, size_t index, const std::shared_ptr<AstNode> &parent)
{
    ASSERT_NE(parent, nullptr);
    ASSERT_TRUE(parent->getChildren().size() != index);
    ASSERT_EQ(parent->getChildren()[index]->getId(), type);
}

void BasicParserTest::expectNodeType(size_t type, const std::shared_ptr<AstNode> &node)
{
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->getId(), type);
}

std::shared_ptr<AstNode>
BasicParserTest::testRule(bool expectsFail, const std::shared_ptr<Rule> &rule, const std::string &input)
{
    std::shared_ptr<SourceManager> sourceManager = std::make_shared<SourceManager>();
    std::shared_ptr<SourceLoggingSink> logger = std::make_shared<SourceLoggingSink>(g_logger.get(), sourceManager);
    std::shared_ptr<BasicTokenizer> tokenizer = std::make_shared<BasicTokenizer>(logger, sourceManager, "TEST");
    std::shared_ptr<AstNode> result = AstNodes::Null().build();
    std::shared_ptr<BasicParsingContext> parser = std::make_shared<BasicParsingContext>(logger, sourceManager);

    sourceManager->addSourceContent("TEST", input);
    bool tokenizeResult = tokenizer->tokenizeBuffer((char *)input.data(), 0, input.size());

    if (!tokenizeResult)
        return nullptr;

    parser->setTokens(tokenizer->getTokens());
    bool parseResult = rule->match(*parser, result) ^ expectsFail;

    if (!parseResult)
    {
        // Show errors on the top of the error queue, this is safe and won't crash because if parsing failed there
        // will be at least 1 error message.
        parser->getErrorCollector()->exitScope(ErrorHandleType::Commit);
    }

    EXPECT_TRUE(parseResult);

    if (!parseResult)
        return nullptr;

    return result;
}
