#ifndef EZPACKER_VIRTUALVARIABLE_H
#define EZPACKER_VIRTUALVARIABLE_H

#include "Combinators.h"
#include "EzFrontendCommon.h"
#include "Identifier.h"
#include "TokenType.h"
#include "Type.h"

namespace grammar
{
/**
 * Creates a rule that tries to match with a virtual variable. Returns a VirtualVariableNode.
 * @return std::shared_ptr<ParseRule>.
 */
extern std::shared_ptr<ParseRule> virtualVariable();
} // namespace grammar

#endif // EZPACKER_VIRTUALVARIABLE_H
