#ifndef EZPACKER_IASTNODEPARSER_H
#define EZPACKER_IASTNODEPARSER_H

#include "EzLexerCommon.h"
#include "AstNode/AstNode.h"

/**
 * Interface that tries to generate an AstNode. The parse method returns a pointer to the node but with the base type
 * of AstNode. It's important to know that CALLER is RESPONSIBLE of saving the current position of the token stream and
 * restoring it if parsing failed.
 */
class IAstNodeParser
{
  public:
    ~IAstNodeParser() = default;

    /**
     * Tries to parse a node out of the given context and returns a pointer to the base node class (AstNode).
     * @param ctx
     * @return std::shared_ptr<AstNode>
     */
    virtual std::shared_ptr<AstNode> parse(const std::shared_ptr<class BasicParsingContext> &ctx) = 0;
};

#endif // EZPACKER_IASTNODEPARSER_H
