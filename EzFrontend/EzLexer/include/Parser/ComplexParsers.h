#ifndef EZPACKER_COMPLEXPARSERS_H
#define EZPACKER_COMPLEXPARSERS_H

#include "PrimitiveParsers.h"
#include "MemoryParsers.h"

/**
 * This file contains parsing rules that require 1 or more PrimitiveParsers or TokenParsers. Parsing rules in this file
 * should be near the top of the AST.
 */

namespace NodeParsers
{
using ParsingRule = std::shared_ptr<Rule>;
extern ParsingRule &InstructionOperand();   // Parses an operand of an instruction (string, number, %virtual variable).
extern ParsingRule &Instruction();          // Parses an instruction with 0, 1 or N operands.
extern ParsingRule &InstructionNoOperand(); // Parses an instruction with 0 operands.
extern ParsingRule &Instruction1OrMoreOperands(); // Parses an instruction with 1 or more operands.
extern ParsingRule &Label();                      // Parses a label (labelName: instructions...)

extern ParsingRule &ModuleHeader(); // Parses the header of a module (.type MyMod (.type1 %arg1, ...)
extern ParsingRule &Module();       // Parses a module, including its header and body.
}; // namespace NodeParsers

#endif // EZPACKER_COMPLEXPARSERS_H
