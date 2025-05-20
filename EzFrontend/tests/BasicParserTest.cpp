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
    std::unique_ptr<BasicTokenizer> tokenizer = std::make_unique<BasicTokenizer>();
    bool tokenizeResult = tokenizer->tokenize((char *)input.data(), 0, input.size());

    if (!tokenizeResult)
        return nullptr;

    std::shared_ptr<Ast> result = std::make_shared<TokenTypeNode>();
    std::unique_ptr<BasicParser> parser = std::make_unique<BasicParser>(tokenizer->getTokens());

    bool parseResult = rule->matchRet(*parser, result) ^ expectsFail;
    EXPECT_TRUE(parseResult);

    if (!parseResult)
        return nullptr;

    return result;
}
