#ifndef EZPACKER_NUMBER_H
#define EZPACKER_NUMBER_H

#include "EzFrontendCommon.h"
#include "ast/ValueNode.h"
#include "grammar/Combinators.h"
#include "grammar/TokenType.h"

namespace grammar
{
/**
 * Creates a rule that tries to match a number, specifically a FLOAT and returns a ValueNode if succeeded.
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> numberFloat();

/**
 * Creates a rule that tries to match a number, specifically an INT and returns a ValueNode if succeeded.
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> numberInt();

/**
 * Creates a rule that tries to match to a number of the given type.
 * @param type
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> number();
} // namespace grammar

#endif // EZPACKER_NUMBER_H
