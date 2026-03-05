/**
 * @file ParserBatch.h
 * @brief Ordered collection of parsers that tries each one until a match is found.
 *
 * ParserBatch is the main entry point for parsing a piece of source code.
 * Parsers are tried in registration order; for each one the stream position
 * and error scope are saved, and automatically rolled back on failure.
 * The first parser that produces a non-null AstNode wins.
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
 * This class represents a batch of parsers that will be tried in the input, by order, and will return the first match.
 */
class ParserBatch
{
  public:
    /**
     * Adds a parser to the batch. If parses is null an exception is thrown.
     * @param parser
     */
    void addParser(std::shared_ptr<IAstNodeParser> parser);

    /**
     * Uses a type list to create and add (using void addParser(std::shared_ptr<IAstNodeParser> parser)) them to this
     * batch.
     * @tparam ParserTypes
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
     * Tries each parser in order against the current token stream. For each parser:
     *   1. The current stream position is saved and a new error scope is begun.
     *   2. If the parser succeeds (returns a non-null node), its error scope is discarded and the result is
     *      returned immediately. If the node has no source reference, one is assigned from the context.
     *   3. If the parser fails with a fatal error, the error scope is propagated upward and no further
     *      parsers are attempted.
     *   4. If the parser fails without a fatal error, the error scope is discarded, the stream position is
     *      restored, and the next parser is tried.
     *
     * Returns {nullptr, nullptr} if no parser matched or if a fatal error occurred.
     * @param ctx
     * @return ParserBatchResult
     */
    ParserBatchResult parse(const std::shared_ptr<BasicParsingContext> &ctx);

  private:
    std::list<std::shared_ptr<IAstNodeParser>> m_parsers;
};

#endif // EZPACKER_PARSERBATCH_H
