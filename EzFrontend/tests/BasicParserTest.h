#ifndef EZPACKER_BASICTESTPARSER_H
#define EZPACKER_BASICTESTPARSER_H

#include "EzFrontendCommon.h"
#include "parser/grammar/Grammar.h"
#include "parser/BasicParser.h"
#include "gtest/gtest.h"

class BasicParserTest : public ::testing::Test
{
  public:
    /**
     * Executes before a test body.
     */
    void SetUp() override;

    /**
     * Executes after a test body.
     */
    void TearDown() override;

    /**
     * Creates the tokenizer, tokenizes input and executes the given parse rule. Returns the result in an Ast node.
     * @param expectsFail
     * @param rule
     * @param input
     */
    static std::shared_ptr<Ast> testRule(bool expectsFail, const std::shared_ptr<ParseRule> &rule,
                                         const std::string &input);
};

#endif // EZPACKER_BASICTESTPARSER_H
