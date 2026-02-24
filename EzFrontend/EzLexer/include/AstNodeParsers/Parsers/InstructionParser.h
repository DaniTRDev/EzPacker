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
     * Tries to parse a CallInstructionParser out of the given context.
     * @return std::shared_ptr<::CallInstructionParser>
     */
    std::shared_ptr<AstNode> parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

class NonCallInstructionParser : public IAstNodeParser
{
  public:
    /**
     * Tries to parse a regular instruction out of the given context.
     * @return std::shared_ptr<Instruction>
     */
    std::shared_ptr<AstNode> parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

class InstructionParser : public IAstNodeParser
{
  public:
    /**
     * Tries to parse an instruction (CallInstructionParser, NonCallInstructionParser) out of the given context.
     * @return std::shared_ptr<Instruction>
     */
    std::shared_ptr<AstNode> parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};
}; // namespace InstructionParser
#endif // EZPACKER_INSTRUCTIONPARSER_H
