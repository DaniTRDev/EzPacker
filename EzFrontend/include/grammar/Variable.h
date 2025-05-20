#ifndef EZPACKER_VARIABLE_H
#define EZPACKER_VARIABLE_H

#include "Combinators.h"
#include "Identifier.h"
#include "Keyword.h"
#include "Number.h"
#include "String.h"
#include "TokenType.h"
#include "Type.h"

namespace grammar
{
/**
 * Tries to parse a list of initializers: initializer1, initializer2, ...
 * Supported types are Int, Float or String.
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> initializer();
/**
 * Creates a rule that tries to match with a variable:
 *  .variable name: .type initializer1, initializer2, ...
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> variable();
} // namespace grammar

#endif // EZPACKER_VARIABLE_H
