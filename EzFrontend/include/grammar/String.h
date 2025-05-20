#ifndef EZPACKER_STRING_H
#define EZPACKER_STRING_H

#include "EzFrontendCommon.h"
#include "ast/ValueNode.h"
#include "grammar/Combinators.h"
#include "grammar/TokenType.h"

namespace grammar
{
/**
 * Creates a rule that matches a string.
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> string();
} // namespace grammar

#endif // EZPACKER_STRING_H
