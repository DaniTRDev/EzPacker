#ifndef EZPACKER_BASICTESTPARSER_H
#define EZPACKER_BASICTESTPARSER_H

#include "Parser/BasicParsingContext.h"
#include "EzLexerCommon.h"
#include "Parser/Parsers.h"
#include "gtest/gtest.h"

/**
 * This class represents a very basic test of the parser. It should be expanded + automatized (generated expect...
 * depending on the definition of each node).
 */
class BasicParserTest : public ::testing::Test
{
  public:
    /**
     * Throws an exception if the given node HASN'T got at least [count] children.
     * @param count
     * @param parent
     */
    static void expectChildCount(size_t count, const std::shared_ptr<AstNode> &parent);

    /**
     * Throws an exception if child at index is NOT of given type.
     * @param type
     * @param index
     * @param parent
     */
    static void expectChildNodeType(size_t type, size_t index, const std::shared_ptr<AstNode> &parent);

    /**
     * Throws an exception if given node is NOT of the given type.
     * @param type
     * @param node
     */
    static void expectNodeType(size_t type, const std::shared_ptr<AstNode> &node);

    /**
     * Executes before a test body.
     */
    void SetUp() override;

    /**
     * Executes after a test body.
     */
    void TearDown() override;

    /**
     * Creates the tokenizer, tokenizes input and executes the given parse rule. Returns the result in an AstNode node.
     * @param expectsFail
     * @param rule
     * @param input
     */
    static std::shared_ptr<AstNode>
    testRule(bool expectsFail, const std::shared_ptr<Rule> &rule, const std::string &input);
};

#endif // EZPACKER_BASICTESTPARSER_H
