#ifndef EZPACKER_REGISTER_H
#define EZPACKER_REGISTER_H

#include "Combinators.h"
#include "EzFrontendCommon.h"
#include "Identifier.h"
#include "TokenType.h"

namespace grammar
{
/**
 * Creates a rule that tries to match with a register. Returns a RegisterNode.
 * @return std::shared_ptr<ParseRule>.
 */
extern std::shared_ptr<ParseRule> _register();
} // namespace grammar

#endif // EZPACKER_REGISTER_H
