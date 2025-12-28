#ifndef EZPACKER_PARSER_H
#define EZPACKER_PARSER_H

#include "EzLexerCommon.h"
#include "AstNode/AstNode.h"
#include "IParsingContext.h"

/**
 * Interface that will define an entry point for each parser. It's important to know that CALLER is RESPONSIBLE
 * of saving the current position of the token stream and restoring it if parsing failed.
 *
 * @tparam AstNodeType Upper type of the parsed node.
 */
template <typename AstNodeType> class IAstNodeParser
{
  public:
    ~IAstNodeParser() = default;

    /**
     * Tries to parse a node out of the given context.
     * @return std::shared_ptr<AstNode>
     */
    virtual std::shared_ptr<AstNodeType> parse(const std::shared_ptr<IParsingContext> &ctx) = 0;
};

#endif // EZPACKER_PARSER_H
