/**
 * @file TypeCheckVisitor.h
 * @brief Third semantic pass: validate structure, casts and operand type use.
 *
 * This pass expects a fully defined and resolved AST. It does not create
 * scopes or resolve names; instead it validates that the previously attached
 * semantic information is used consistently.
 *
 * Observable effects of this visitor:
 *   - Validates `break` / `continue` against the current loop/switch nesting.
 *   - Checks instruction operands against the inferred target type when that
 *     type can be derived from another operand.
 *   - Validates explicit variable type uses and, when needed, replaces the
 *     plain symbol view with `TypeCastAnnotation` so lowering knows which cast
 *     to emit.
 *   - Checks immediates and switch-case literals against the destination type
 *     and annotates them with a cast target when required for lowering.
 *
 * After this pass succeeds, the AST is considered semantically valid for the
 * public lowering entry points available in EzSemantics.
 */
#ifndef EZPACKER_TYPECHECKVISITOR_H
#define EZPACKER_TYPECHECKVISITOR_H

#include "EzSemanticsCommon.h"
#include "BasicSemanticContext.h"
#include "SemanticVisitor.h"
#include "Scope/TypeTable.h"
#include "SemanticAnnotations/DataTypeAnnotation.h"
#include "SemanticAnnotations/SymbolAnnotation.h"
#include "SemanticAnnotations/TypeCastAnnotation.h"

class TypeCheckVisitor : public SemanticVisitor
{
  public:
    /**
     * Visits a `break` statement.
     *
     * `break` is legal inside either a loop or a `switch`.
     */
    bool visit(BreakAstNode *_break) override;

    /**
     * Visits a lexical code scope and validates each child expression.
     */
    bool visit(CodeScope *scope) override;

    /**
     * Visits a `continue` statement.
     *
     * `continue` is legal only inside a loop.
     */
    bool visit(ContinueAstNode *_continue) override;

    /**
     * Visits a `for` loop.
     *
     * The initialiser, condition and next-iteration clause are validated first;
     * the loop body is then validated while the semantic context is marked as
     * being inside a loop.
     */
    bool visit(ForAstNode *_for) override;

    /**
     * Visits an `if` statement and validates its condition plus both branches.
     */
    bool visit(IfAstNode *ifNode) override;

    /**
     * Visits an instruction.
     *
     * The visitor derives a target operand type when possible and uses it to
     * validate immediates and operand compatibility across the instruction.
     */
    bool visit(Instruction *instr) override;

    /**
     * Visits a label and validates all expressions in its body.
     */
    bool visit(Label *label) override;

    /**
     * Visits a module and validates all expressions in its body.
     */
    bool visit(Module *module) override;

    /**
     * Visits a memory operand.
     *
     * The memory reference must already have a resolved element type. Any
     * variables used in its addressing mode are validated recursively.
     */
    bool visit(MemoryOperandAstNode *operand) override;

    /**
     * Visits a variable use.
     *
     * If the use explicitly requests a different type than the symbol's
     * declared type, cast safety is checked and a `TypeCastAnnotation` is
     * attached for the lowerer.
     */
    bool visit(Variable *var) override;

    /**
     * Visits a `switch` statement.
     *
     * The controlling variable is validated first, then each case literal is
     * checked against the switch variable type while the semantic context is
     * marked as being inside a switch.
     */
    bool visit(SwitchAstNode *_switch) override;

    /**
     * Visits a single switch case and validates its body.
     */
    bool visit(SwitchCaseAstNode *_switchCase) override;

    /**
     * Visits a `while` loop.
     *
     * The condition is validated first; the body is then validated while the
     * semantic context is marked as being inside a loop.
     */
    bool visit(WhileAstNode *whileNode) override;

  private:
    /**
     * Checks whether using `usedType` in place of `originalType` is legal for
     * the given node.
     *
     * Fatal errors are emitted for unsupported conversions; lossy but allowed
     * conversions produce warnings.
     */
    bool checkCastSafety(AstNode *node, Type *originalType, Type *usedType);

    /**
     * Checks whether the given immediate literal can be represented by the
     * requested target type.
     */
    bool checkImmediateSafety(ImmediateOperand *operand, Type *usedType);
};

#endif // EZPACKER_TYPECHECKVISITOR_H
