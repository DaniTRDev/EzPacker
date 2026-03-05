/**
 * @file MemoryOperandParser.h
 * @brief Parsers for the various memory-addressing modes.
 *
 * Four parser classes live inside the MemoryOperandParser namespace,
 * each recognising a different addressing pattern:
 *   - BaseDisplacement            — `type (%base + disp)`
 *   - BaseIndexScaleDisplacement  — `type (%base, %idx, scale, disp)`
 *   - IndexScale                  — `type (, %idx, scale)`
 *   - Direct                      — `type (address)`
 *
 * The top-level MemoryOperandParser tries each sub-parser in order and
 * returns the first match as a MemoryOperandAstNode subclass.
 */
#ifndef EZPACKER_MEMORYOPERANDPARSER_H
#define EZPACKER_MEMORYOPERANDPARSER_H

#include "EzLexerCommon.h"
#include "AstNodeParsers/IAstNodeParser.h"
#include "AstNodes/MemoryOperand.h"
#include "VariableParser.h"
#include "ImmediateParser.h"

namespace MemoryOperandParser
{
class BaseDisplacement : public IAstNodeParser
{
  public:
    /**
     * Tries to parse a BaseDisplacement memory operand out of the token list within context.
     * @param ctx
     * @return BaseDisplacementMemory*
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

class BaseIndexScaleDisplacement : public IAstNodeParser
{
  public:
    /**
     * Tries to parse a BaseIndexScaleDisplacement memory operand out of the token list within context.
     * @param ctx
     * @return BaseIndexScaleDisplacement*
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

class IndexScale : public IAstNodeParser
{
  public:
    /**
     * Tries to parse a IndexScale memory operand out of the token list within context.
     * @param ctx
     * @return IndexScale*
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

class Direct : public IAstNodeParser
{
  public:
    /**
     * Tries to parse a DirectMemory operand out of the token list within context.
     * @param ctx
     * @return DirectMemory*
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

class MemoryOperandParser : public IAstNodeParser
{
  public:
    /**
     * Tries to parse a memory operand out of the token list within context.
     * @param ctx
     * @return AstNode*
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};
}; // namespace MemoryOperandParser

#endif // EZPACKER_MEMORYOPERANDPARSER_H
