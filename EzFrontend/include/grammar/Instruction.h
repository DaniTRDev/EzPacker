#ifndef EZPACKER_INSTRUCTION_H
#define EZPACKER_INSTRUCTION_H

#include "Addressing.h"
#include "EzFrontendCommon.h"
#include "String.h"

namespace grammar
{
/**
 * Creates a parse rule that tries to parse an operand: register, immediate or memory. Returns nullptr if failed.
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> operand();

/**
 * Creates a rule that tries to match given tokens with an instruction. An instruction contains:
 * .instructionKeyword .type operand1, operand2. Sometimes operand1, operand2 or both might not be present, parser alone
 * does not know this, it's a responsibility of the semantic engine to determine if operand(s) are source or
 * destination.
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> instruction();

} // namespace grammar

#endif // EZPACKER_INSTRUCTION_H
