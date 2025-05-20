#ifndef EZPACKER_MODULE_H
#define EZPACKER_MODULE_H

#include "EzFrontendCommon.h"
#include "grammar/Keyword.h"
#include "grammar/Instruction.h"

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
}

#endif // EZPACKER_MODULE_H
