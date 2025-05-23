#ifndef EZPACKER_IDENTIFIER_H
#define EZPACKER_IDENTIFIER_H

#include "Combinators.h"
#include "EzFrontendCommon.h"
#include "TokenType.h"
#include "parser/ast/IdentifierNode.h"

namespace grammar
{
/**
 * Checks for the current token and returns an IdentifierNode if token was an identifier.
 * @param type
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> identifier();
} // namespace Grammar

#endif // EZPACKER_IDENTIFIER_H
