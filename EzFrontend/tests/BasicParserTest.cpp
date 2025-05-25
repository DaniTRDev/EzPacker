#include "BasicParserTest.h"

void BasicParserTest::SetUp()
{
}

void BasicParserTest::TearDown()
{
    Test::TearDown();
}

std::shared_ptr<Ast> BasicParserTest::testRule(bool expectsFail, const std::shared_ptr<ParseRule> &rule,
                                               const std::string &input)
{
    std::shared_ptr<SourceManager> sourceManager = std::make_shared<SourceManager>();
    std::shared_ptr<FrontendLogger> logger = std::make_shared<FrontendLogger>(sourceManager);
    std::shared_ptr<BasicTokenizer> tokenizer = std::make_shared<BasicTokenizer>(sourceManager, logger, "TEST");

    sourceManager->addSourceContent("TEST", input);
    bool tokenizeResult = tokenizer->tokenize((char *)input.data(), 0, input.size());

    if (!tokenizeResult)
        return nullptr;

    std::shared_ptr<Ast> result = std::make_shared<TokenTypeNode>();
    std::shared_ptr<BasicParser> parser = std::make_shared<BasicParser>(logger, sourceManager, tokenizer->getTokens());

    bool parseResult = rule->matchRet(*parser, result) ^ expectsFail;
    EXPECT_TRUE(parseResult);

    if (!parseResult)
        return nullptr;

    return result;
}
