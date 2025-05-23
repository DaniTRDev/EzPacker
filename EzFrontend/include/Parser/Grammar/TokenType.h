#ifndef EZPACKER_TOKENTYPE_H
#define EZPACKER_TOKENTYPE_H

#include "EzFrontendCommon.h"
#include "parser/ParseRule.h"
#include "parser/ast/TokenTypeNode.h"

namespace grammar
{
/**
 * Checks for the given type and returns true, must be used only to check if a node if a token is present because it
 * won't push any node.
 * @param type
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> tokenType(IRTokenType type);
} // namespace Grammar

#endif // EZPACKER_TOKENTYPE_H
