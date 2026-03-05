/**
 * @file InstructionParser.h
 * @brief Parsers for assembly-style instructions: `mnemonic op1, op2;`.
 *
 * Three parser classes live inside the InstructionParser namespace:
 *   - CallInstructionParser    — parses `call` instructions that invoke
 *                                 another module/function.
 *   - NonCallInstructionParser — parses regular instructions (mov, add, …)
 *                                 with their variable/immediate/memory operands.
 *   - InstructionParser        — top-level parser that tries call first,
 *                                 then falls back to non-call.
 */
#ifndef EZPACKER_INSTRUCTIONPARSER_H
#define EZPACKER_INSTRUCTIONPARSER_H

#include "EzLexerCommon.h"
#include "AstNodeParsers/IAstNodeParser.h"
#include "AstNodes/Instruction.h"
#include "VariableParser.h"
#include "ImmediateParser.h"
#include "MemoryOperandParser.h"

namespace InstructionParser
{
class CallInstructionParser : public IAstNodeParser
{
  public:
    /**
     * Tries to parse a call instruction out of the given context.
     * @return InstructionAstNode
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

class NonCallInstructionParser : public IAstNodeParser
{
  public:
    /**
     * Tries to parse a regular instruction out of the given context.
     * @return AstNode *
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

class InstructionParser : public IAstNodeParser
{
  public:
    /**
     * Tries to parse an instruction (CallInstructionParser, NonCallInstructionParser) out of the given context.
     * @return AstNode *
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};
}; // namespace InstructionParser
#endif // EZPACKER_INSTRUCTIONPARSER_H
