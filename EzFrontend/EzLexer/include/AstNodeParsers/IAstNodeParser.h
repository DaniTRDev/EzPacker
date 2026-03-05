/**
 * @file IAstNodeParser.h
 * @brief Interface that every individual language-construct parser implements.
 *
 * A parser attempts to consume tokens from the stream and produce a single
 * AstNode.  When used through ParserBatch the stream position is
 * automatically saved and restored on failure; when used standalone the
 * caller is responsible for that bookkeeping.
 */
#ifndef EZPACKER_IASTNODEPARSER_H
#define EZPACKER_IASTNODEPARSER_H

#include "EzLexerCommon.h"
#include "AstNode/AstNode.h"

/**
 * Interface that tries to generate an AstNode from a token stream. The parse method returns a pointer to the
 * node (as AstNode *), or nullptr on failure. When used through ParserBatch, the batch automatically saves
 * and restores the token stream position on parser failure. When used directly (without ParserBatch), the
 * CALLER is responsible for saving and restoring the token stream position if parsing fails.
 */
class IAstNodeParser
{
  public:
    ~IAstNodeParser() = default;

    /**
     * Tries to parse a node out of the given context and returns a pointer to the base node class (AstNode).
     * @param ctx
     * @return AstNode*
     */
    virtual AstNode *parse(const std::shared_ptr<class BasicParsingContext> &ctx) = 0;
};

#endif // EZPACKER_IASTNODEPARSER_H
