/**
 * @file ParserBatch.h
 * @brief Ordered parser dispatcher with automatic rollback on non-fatal misses.
 *
 * ParserBatch is the safest way to combine multiple grammar alternatives. It
 * preserves the token cursor when a parser simply does not match, but stops
 * immediately when a parser reports a fatal error. This lets individual parser
 * implementations stay focused on one construct while the batch handles the
 * backtracking policy.
 */
#ifndef EZPACKER_PARSERBATCH_H
#define EZPACKER_PARSERBATCH_H

#include "EzLexerCommon.h"
#include "AstNodeParsers/IAstNodeParser.h"
#include "BasicParsingContext.h"

struct ParserBatchResult
{
    AstNode *m_node;
    std::shared_ptr<IAstNodeParser> m_parser;
};

/**
 * Ordered collection of parsers evaluated against the same token stream.
 *
 * Registration order is part of the contract: if two parsers can begin with
 * the same token prefix, the earlier one gets priority. This is why specialized
 * parsers such as `CallInstructionParser` are typically registered before more
 * generic ones.
 */
class ParserBatch
{
  public:
    /**
     * Adds one parser instance to the end of the batch.
     *
     * @throws std::runtime_error if parser is null.
     */
    void addParser(std::shared_ptr<IAstNodeParser> parser);

    /**
     * Convenience helper that default-constructs and appends parser types in
     * the order they appear in the template parameter pack.
     */
    template <typename FirstParserType, typename... ParserTypes>
        requires(std::is_base_of<IAstNodeParser, FirstParserType>::value)
    void addParsersFromTypeList()
    {
        if constexpr (sizeof...(ParserTypes) > 0)
        {
            addParser(std::make_shared<FirstParserType>());
            addParsersFromTypeList<ParserTypes...>();
        }
        else
        {
            addParsersFromTypeList<FirstParserType, true>();
        }
    }

    /**
     * Uses a type list to create and add (using void addParser(std::shared_ptr<IAstNodeParser> parser)) them to this
     * batch.
     * @tparam ParserTypes
     */
    template <typename FirstParserType, bool UnAmbiguator>
        requires(std::is_base_of<IAstNodeParser, FirstParserType>::value)
    void addParsersFromTypeList()
    {
        addParser(std::make_shared<FirstParserType>());
    }

    /**
     * Tries each parser in registration order against the current token stream.
     *
     * Success contract:
     * - returns the produced node and the parser instance that produced it,
     * - leaves the context cursor advanced past the matched construct,
     * - assigns a source reference from the context if the node did not set one.
     *
     * Failure contract:
     * - soft failure from one parser rolls the cursor back and continues with
     *   the next parser,
     * - fatal failure stops the batch immediately and propagates that error
     *   scope upward,
     * - if no parser matches, the returned node and parser are both null.
     */
    ParserBatchResult parse(const std::shared_ptr<BasicParsingContext> &ctx);

  private:
    std::list<std::shared_ptr<IAstNodeParser>> m_parsers;
};

#endif // EZPACKER_PARSERBATCH_H
