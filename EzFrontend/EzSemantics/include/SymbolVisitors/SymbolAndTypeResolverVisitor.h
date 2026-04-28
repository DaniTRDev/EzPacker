/**
 * @file SymbolAndTypeResolverVisitor.h
 * @brief Second semantic pass: resolve name uses and concrete data types.
 *
 * This visitor expects the AST to have already been processed by
 * `SymbolDefinitionVisitor`. It re-enters the scopes created in that first
 * pass and turns unresolved syntax into explicit semantic links.
 *
 * Observable effects of this visitor:
 *   - Resolves variable uses to the `Symbol` that defines them.
 *   - Resolves explicit type names on immediates and memory operands through
 *     `TypeTable`.
 *   - Applies default types where the syntax omitted them (`i64` for integer
 *     immediates, `double` for floating-point immediates, and the language
 *     default type for untyped memory operands).
 *   - Attaches `SymbolAnnotation`, `DataTypeAnnotation` and consumes
 *     `ScopeAnnotation` / `ScopedSymbolAnnotation` produced by the first pass.
 *
 * This pass does not enforce cast safety or structural rules such as whether
 * `break` is legal in the current context; that belongs to
 * `TypeCheckVisitor`.
 */
#ifndef EZPACKER_SYMBOLANDTYPERESOLVERVISITOR_H
#define EZPACKER_SYMBOLANDTYPERESOLVERVISITOR_H

#include "EzSemanticsCommon.h"
#include "BasicSemanticContext.h"
#include "SemanticVisitor.h"
#include "Scope/Scope.h"
#include "SemanticAnnotations/DataTypeAnnotation.h"
#include "SemanticAnnotations/ScopeAnnotation.h"
#include "SemanticAnnotations/ScopedSymbolAnnotation.h"
#include "SemanticAnnotations/SymbolAnnotation.h"

/**
 * Semantic pass that resolves symbols and concrete operand types.
 */
class SymbolAndTypeResolverVisitor : public SemanticVisitor
{
  public:
    /**
     * Visits a lexical code scope and resolves all child expressions in the
     * already active scope.
     */
    bool visit(CodeScope *scope) override;

    /**
     * Visits a condition and resolves both operands that participate in the
     * comparison.
     */
    bool visit(ConditionAstNode *cond) override;

    /**
     * Visits a `for` loop.
     *
     * The visitor re-enters the scope attached to the `ForAstNode` during the
     * definition pass so the initialiser, condition, next-iteration clause and
     * body see the same declarations.
     */
    bool visit(ForAstNode *_for) override;

    /**
     * Visits an `if` statement.
     *
     * The condition is resolved in the current scope. Each branch body is then
     * resolved inside the dedicated scope attached to that branch's
     * `CodeScope`.
     */
    bool visit(IfAstNode *ifNode) override;

    /**
     * Visits an immediate literal.
     *
     * If the source explicitly specified a type name, that type is resolved and
     * attached as a `DataTypeAnnotation`. Otherwise a default type is inferred
     * from the literal kind.
     */
    bool visit(ImmediateOperand *imm) override;

    /**
     * Visits an instruction.
     *
     * Ordinary operands are resolved recursively. The declaration instruction
     * `create` is treated as already fully handled by `SymbolDefinitionVisitor`
     * and therefore skipped here.
     */
    bool visit(Instruction *instr) override;

    /**
     * Visits a label definition and resolves its body inside the scope owned by
     * the label.
     */
    bool visit(Label *label) override;
    
    /**
     * Visits a module and resolves its body inside the scope owned by the
     * module.
     */
    bool visit(Module *module) override;

    /**
     * Visits a variable use.
     *
     * If the node already carries a `SymbolAnnotation` (for example because it
     * is also the declaration site of a global or local definition), no extra
     * lookup is performed. Otherwise the visitor resolves the name against the
     * active scope chain and attaches the matching symbol.
     */
    bool visit(Variable *var) override;

    /**
     * Visits a `switch` statement, resolving the controlling variable and then
     * each case.
     */
    bool visit(SwitchAstNode *_switch) override;

    /**
     * Visits a single switch case inside the scope attached to that case.
     *
     * Both the case value and the case body are resolved there.
     */
    bool visit(SwitchCaseAstNode *switchCase) override;

    /**
     * Visits a `while` loop.
     *
     * The condition is resolved in the enclosing scope, then the body is
     * resolved inside the scope attached to the `WhileAstNode`.
     */
    bool visit(WhileAstNode *whileNode) override;
};

#endif // EZPACKER_SYMBOLANDTYPERESOLVERVISITOR_H
