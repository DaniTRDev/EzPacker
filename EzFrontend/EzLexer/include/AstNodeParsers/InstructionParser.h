#ifndef EZPACKER_INSTRUCTIONPARSER_H
#define EZPACKER_INSTRUCTIONPARSER_H

#include "EzLexerCommon.h"
#include "AstNodeParser/IAstNodeParser.h"
#include "AstNodes/Instruction.h"
#include "AstNodeParser/AstNodeParsingUtils.h"
#include "AstNodeParsers/VariableParser.h"
#include "AstNodeParsers/ImmediateOperandParser.h"
#include "AstNodeParsers/MemoryOperandParser.h"

class InstructionParser : public IAstNodeParser<Instruction>
{
  public:
    /**
     * Tries to parse an instruction out of the given context.
     * @return std::shared_ptr<Instruction>
     */
    std::shared_ptr<Instruction> parse(const std::shared_ptr<IParsingContext> &ctx) override;

    class CallInstructionParser : public IAstNodeParser<CallInstruction>
    {
      public:
        /**
         * Tries to parse a CallInstructionParser out of the given context.
         * @return std::shared_ptr<::CallInstructionParser>
         */
        std::shared_ptr<CallInstruction> parse(const std::shared_ptr<IParsingContext> &ctx) override;
    };

    class RegularInstruction : public IAstNodeParser<Instruction>
    {
      public:
        /**
         * Tries to parse a regular instruction out of the given context.
         * @return std::shared_ptr<Instruction>
         */
        std::shared_ptr<Instruction> parse(const std::shared_ptr<IParsingContext> &ctx) override;
    };
};

#endif // EZPACKER_INSTRUCTIONPARSER_H
