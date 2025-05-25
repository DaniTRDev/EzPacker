#ifndef EZPACKER_MODULE_H
#define EZPACKER_MODULE_H

#include "EzFrontendCommon.h"
#include "Instruction.h"
#include "Keyword.h"

namespace grammar
{
/**
 * Creates a rule that tries to match a module:
 * .module name:
 *   instructions (if any) ....
 * .end
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> module();
/**
 * Creates a rule that tries to match a single module parameter:
 * (type Param1, type Param2, ...)
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> parameter();
}

#endif // EZPACKER_MODULE_H
