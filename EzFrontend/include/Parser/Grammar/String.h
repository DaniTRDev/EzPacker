#ifndef EZPACKER_STRING_H
#define EZPACKER_STRING_H

#include "Combinators.h"
#include "EzFrontendCommon.h"
#include "TokenType.h"
#include "parser/ast/ValueNode.h"

namespace grammar
{
/**
 * Creates a rule that matches a string.
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> string();
} // namespace Grammar

#endif // EZPACKER_STRING_H
