/**
 * @file TypeCheckVisitor.h
 * @brief Third semantic pass — validates type compatibility and structural rules.
 *
 * TypeCheckVisitor walks the fully-resolved AST and:
 *   - Verifies that instruction operand types are compatible (e.g. both
 *     sides of an ADD must have the same bit-width, or an implicit cast
 *     must be possible).
 *   - Replaces a SymbolAnnotation with a TypeCastAnnotation when a variable
 *     is used with a type different from its declared type.
 *   - Rejects break/continue statements that appear outside of a while-loop.
 *   - Validates memory operand types and condition operand types.
 *
 * After this pass succeeds the AST is fully validated and ready for
 * lowering to MIR.
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
     * Visits given BreakAstNode node. It will throw an error if the node is not inside a loop.
     * @param _break
     * @return bool
     */
    bool visit(BreakAstNode *_break) override;
    
    /**
     * Visits given CodeScope node. It will visit its expressions.
     * @param scope
     * @return bool
     */
    bool visit(CodeScope *scope) override;
    
    /**
     * Visits given ContinueAstNode node. It will throw an error if the node is not inside a loop.
     * @param _continue
     * @return bool
     */
    bool visit(ContinueAstNode *_continue) override;

    /**
     * Visits the given IfAstNode. Will try to resolve the symbol and types from the condition and true and false
     * branches.
     * @param ifNode
     * @return bool
     */
    bool visit(IfAstNode *ifNode);

    /**
     * Visits given instruction node. Recursively visits operands.
     * @param instr
     * @return bool
     */
    bool visit(Instruction *instr) override;

    /**
     * Visits given Label node. Recursively visits its expressions.
     * @param label
     * @return bool
     */
    bool visit(Label *label) override;

    /**
     * Visits given Module node. Recursively visits its expressions.
     * @param module
     * @return bool
     */
    bool visit(Module *module) override;

    /**
     * Visits given Memory operand node. Visits used variable nodes (if any).
     * @param operand
     * @return bool
     */
    bool visit(MemoryOperandAstNode *operand) override;

    /**
     * Visits given Variable node. This visitor will check for the types of
     * the symbol this variable references and the used type. If these types do not match will change node's annotation
     * to a TypeCastAnnotation.
     * @param var
     * @return bool
     */
    bool visit(Variable *var) override;

    /**
     * Visits the given WhileAstNode. Will try to resolve the symbol and types from the condition the loop branch.
     * @param whileNode
     * @return bool
     */
    bool visit(WhileAstNode *whileNode) override;

  private:
    /**
     * Performs a type cast safety check and returns true if cast can be done.
     * @param node
     * @param originalType The original type.
     * @param usedType The new type which, if possible, will be used.
     * @return bool
     */
    bool checkCastSafety(AstNode *node, Type *originalType, Type *usedType);

    /**
     * Checks if the given immediate matches the target type or can be casted onto it.
     * @param operand
     * @param usedType
     * @return
     */
    bool checkImmediateSafety(ImmediateOperand *operand, Type *usedType);
};

#endif // EZPACKER_TYPECHECKVISITOR_H
