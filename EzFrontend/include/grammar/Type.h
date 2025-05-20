#ifndef EZPACKER_TYPE_H
#define EZPACKER_TYPE_H

#include "Combinators.h"
#include "EzFrontendCommon.h"
#include "Identifier.h"
#include "TokenType.h"

namespace grammar
{
/**
 * Creates a rule that tries to match with a type and returns a type node if succeeded.
 * @param type
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> type();
} // namespace grammar

#endif // EZPACKER_TYPE_H
