#ifndef EZPACKER_BASICNORMALIZERTEST_H
#define EZPACKER_BASICNORMALIZERTEST_H

#include "EzFrontendCommon.h"
#include "Parser/BasicParser.h"
#include "Parser/Grammar/Grammar.h"
#include "Semantics/Normalizer/ModuleNormalizer.h"
#include "Tokenizer/BasicTokenizer.h"
#include <gtest/gtest.h>

class BasicNormalizerTest : public ::testing::Test
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
    static bool test();
};

#endif // EZPACKER_BASICNORMALIZERTEST_H
