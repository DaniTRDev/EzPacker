/**
 * @file SymbolDefinitionVisitor.h
 * @brief First semantic pass: create scopes and register symbol definitions.
 *
 * This pass builds the symbol/scope skeleton consumed by all later semantic
 * and lowering stages. It does not resolve name uses and it does not perform
 * type-compatibility checks.
 *
 * Observable effects of this visitor:
 *   - Defines symbols for modules, labels, parameters, globals and locals
 *     introduced by the `create` instruction.
 *   - Creates lexical scopes for modules, labels, `if` branches, `while`
 *     bodies, `for` loops and `switch` cases.
 *   - Attaches `SymbolAnnotation`, `ScopeAnnotation` and
 *     `ScopedSymbolAnnotation` to the AST nodes that own those entities.
 *   - Emits diagnostics for invalid declarations and same-scope
 *     redefinitions; shadowing of parent-scope variables is allowed but is
 *     reported as a warning.
 *
 * Expected order of use:
 *   1. Run this visitor before resolution and type checking.
 *   2. Reuse the same `BasicSemanticContext` in later passes so they can
 *      consume the scopes and annotations created here.
 */
#ifndef EZPACKER_SYMBOLDEFINITIONVISITOR_H
#define EZPACKER_SYMBOLDEFINITIONVISITOR_H

#include "EzSemanticsCommon.h"
#include "BasicSemanticContext.h"
#include "SemanticVisitor.h"
#include "Scope/Scope.h"
#include "SemanticAnnotations/ScopeAnnotation.h"
#include "SemanticAnnotations/ScopedSymbolAnnotation.h"
#include "SemanticAnnotations/SymbolAnnotation.h"

/**
 * Semantic pass that builds the initial symbol table and scope tree.
 */
class SymbolDefinitionVisitor : public SemanticVisitor
{
  public:
    /**
     * Visits a lexical code scope and recursively defines symbols in its child
     * expressions.
     */
    bool visit(struct CodeScope *scope) override;

    /**
     * Visits a `for` loop.
     *
     * The pass creates one scope owned by the `ForAstNode` so symbols declared
     * in the initialisation clause remain visible to the condition,
     * next-iteration clause and body.
     */
    bool visit(struct ForAstNode *_for) override;

    /**
     * Visits an `if` statement.
     *
     * The condition is traversed in the current scope. Each branch body gets
     * its own child scope attached to the corresponding `CodeScope` node.
     */
    bool visit(IfAstNode *ifNode) override;

    /**
     * Visits an instruction.
     *
     * Only the `create` instruction mutates semantic state in this pass: it is
     * interpreted as a local-variable declaration and therefore defines one
     * symbol in the current scope.
     */
    bool visit(struct Instruction *instr) override;

    /**
     * Visits a label definition.
     *
     * The label symbol is created in the enclosing scope, while the label body
     * receives its own owned scope. The node is annotated with
     * `ScopedSymbolAnnotation` so later passes can recover both pieces.
     */
    bool visit(struct Label *label) override;

    /**
     * Visits a module header and defines its declared parameters in the
     * module's current scope.
     */
    bool visit(struct ModuleHeader *header) override;

    /**
     * Visits a module definition.
     *
     * The module symbol is defined in the enclosing scope, its return type is
     * validated against `TypeTable`, and a dedicated module scope is created
     * for parameters and body-local declarations.
     */
    bool visit(struct Module *module) override;

    /**
     * Visits a variable declaration node.
     *
     * This overload is used only for declaration contexts, not arbitrary name
     * uses. Variables defined while the current scope is global become
     * `GlobalVariable`; otherwise they become `LocalVariable`.
     */
    bool visit(struct Variable *variable) override;

    /**
     * Visits a `switch` statement.
     *
     * The `switch` node itself does not create a new scope in this pass; each
     * case body does.
     */
    bool visit(SwitchAstNode *_switch) override;

    /**
     * Visits a single `case` inside a switch.
     *
     * Each case owns a dedicated scope so declarations do not leak across
     * sibling cases.
     */
    bool visit(SwitchCaseAstNode *_switch) override;

    /**
     * Visits a `while` loop.
     *
     * The condition is traversed in the enclosing scope and the loop body gets
     * its own child scope attached to the `WhileAstNode`.
     */
    bool visit(WhileAstNode *whileNode) override;
};

#endif // EZPACKER_SYMBOLDEFINITIONVISITOR_H
