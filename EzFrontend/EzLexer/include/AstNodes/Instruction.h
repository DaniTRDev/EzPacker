/**
 * @file Instruction.h
 * @brief AST nodes for regular instructions and the specialized `call` form.
 *
 * An Instruction models one statement terminated by `;` inside a code scope.
 * Operands are stored in source order inside AstNodeContainer.
 *
 * Grammar notes derived from the public parser contract:
 * - Regular instruction: `mnemonic;` or `mnemonic operand[, operand...];`
 * - `mnemonic` is case-insensitive at parse time and is normalized to lower
 *   case before being stored.
 * - Supported operand categories are variables, immediates, and memory
 *   operands.
 * - `CallInstruction` uses a specialized grammar and stores the callee as the
 *   first expression, followed by the argument expressions.
 */
#ifndef EZPACKER_INSTRUCTION_H
#define EZPACKER_INSTRUCTION_H

#include "EzLexerCommon.h"
#include "Variable.h"
#include "ImmediateOperand.h"
#include "AstNode/AstNodeContainer.h"
#include "AstNode/AstNodeVisitor.h"

/**
 * Assembly-style instruction with zero or more operands.
 *
 * Examples:
 * - `nop;`
 * - `add %dst, 1;`
 * - `lea %dst, i64 (%base+0x10);`
 */
class Instruction : public AstNode, public AstNodeContainer
{
  public:
    /**
     * Creates an instruction node.
     *
     * @param operands Operand slice in source order. May be empty, but should
     *                 not be null.
     * @param instructionName Lower-cased instruction mnemonic interned in the
     *                        parsing context's StringPool.
     */
    Instruction(TypedPoolLinkedList<AstNode> *operands, std::string_view instructionName);

    /**
     * Returns AstNodeType::Instruction.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

    /**
     * Accepts the given visitor and calls its internal visit method with the correct node type. Returns
     * the result of visit.
     * @param visitor
     * @return bool
     */
    bool accept(AstNodeVisitor *visitor) override;

    /**
     * Returns "Instruction".
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Returns the normalized mnemonic exactly as stored by the parser.
     */
    const std::string_view &getInstructionName() const;

  private:
    std::string_view m_instructionName;
};

/**
 * Specialized instruction node for function/module calls.
 *
 * Parsed shape:
 * - `call %callee();`
 * - `call %callee(arg1, arg2);`
 *
 * Representation contract:
 * - expression 0 is always the callee variable,
 * - expressions 1..N are the call arguments in source order.
 *
 * EzSemantics is responsible for resolving the callee symbol and validating
 * the argument list.
 */
class CallInstruction : public Instruction
{
  public:
    /**
     * Creates the call instruction.
     *
     * @param params Slice whose first element is the callee and remaining
     *               elements are call arguments.
     */
    CallInstruction(TypedPoolLinkedList<AstNode> *params);
};

#endif // EZPACKER_INSTRUCTION_H
