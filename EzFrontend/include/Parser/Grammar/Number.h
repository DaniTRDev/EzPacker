#ifndef EZPACKER_NUMBER_H
#define EZPACKER_NUMBER_H

#include "Combinators.h"
#include "EzFrontendCommon.h"
#include "TokenType.h"
#include "parser/ast/ValueNode.h"

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
} // namespace Grammar

#endif // EZPACKER_NUMBER_H
