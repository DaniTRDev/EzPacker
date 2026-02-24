#ifndef EZPACKER_PARSERBATCH_H
#define EZPACKER_PARSERBATCH_H

#include "EzLexerCommon.h"
#include "AstNodeParsers/IAstNodeParser.h"
#include "BasicParsingContext.h"

struct ParserBatchResult
{
    std::shared_ptr<AstNode> m_node;
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
     * Tries to apply set parsers, if any, in the given context. Returns in the first match. This function will create
     * 1 scope within the error collector of the given context. In this scope other parsers will also begin/end a new
     * scope in which their errors, if any, will be pushed.
     *
     * IMPORTANT: If there's a match, errors will be DISCARDED.
     * @param ctx
     * @return ParserBatchResult
     */
    ParserBatchResult parse(const std::shared_ptr<BasicParsingContext> &ctx);

  private:
    std::list<std::shared_ptr<IAstNodeParser>> m_parsers;
};

#endif // EZPACKER_PARSERBATCH_H
