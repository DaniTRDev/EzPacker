#ifndef EZPACKER_KEYWORD_H
#define EZPACKER_KEYWORD_H

#include "Combinators.h"
#include "EzFrontendCommon.h"
#include "Identifier.h"
#include "TokenType.h"

namespace grammar
{
/**
 * Creates a rule that tries to match with a keyword. Returns true or false but doesn't push anything to the out node
 * vector.
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> keyword();
} // namespace Grammar

#endif // EZPACKER_KEYWORD_H
