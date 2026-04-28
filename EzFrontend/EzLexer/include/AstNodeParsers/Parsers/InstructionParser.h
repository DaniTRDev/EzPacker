/**
 * @file InstructionParser.h
 * @brief Parsers for statement-level instructions.
 *
 * Supported source forms:
 * - regular instruction: `mnemonic;`
 * - regular instruction with operands: `mnemonic op1, op2;`
 * - call instruction: `call %callee()` or `call %callee(arg1, arg2);`
 *
 * Operand parsers are tried in this order: immediate, variable, memory operand.
 * This matters when multiple operand syntaxes share a prefix.
 */
#ifndef EZPACKER_INSTRUCTIONPARSER_H
#define EZPACKER_INSTRUCTIONPARSER_H

#include "EzLexerCommon.h"
#include "AstNodeParsers/IAstNodeParser.h"
#include "AstNodes/Instruction.h"
#include "VariableParser.h"
#include "ImmediateParser.h"

namespace InstructionParser
{
class CallInstructionParser : public IAstNodeParser
{
  public:
    /**
     * Parses the specialized `call` form.
     *
     * On success, the returned node is a CallInstruction whose expression list
     * begins with the callee variable followed by zero or more arguments.
     * The parser expects the call to be terminated by `;`.
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

class NonCallInstructionParser : public IAstNodeParser
{
  public:
    /**
     * Parses any non-`call` instruction.
     *
     * The mnemonic must be an identifier. Zero-operand instructions must still
     * end with `;`. Operands, when present, are comma-separated and parsed in
     * source order.
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

class InstructionParser : public IAstNodeParser
{
  public:
    /**
     * Tries `CallInstructionParser` first and falls back to
     * `NonCallInstructionParser` on a non-fatal miss.
     *
     * This ordering ensures that the `call` keyword is handled by its special
     * grammar before the generic instruction parser can consume it.
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};
}; // namespace InstructionParser
#endif // EZPACKER_INSTRUCTIONPARSER_H
