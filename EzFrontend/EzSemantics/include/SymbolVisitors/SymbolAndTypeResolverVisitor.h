/**
 * @file SymbolAndTypeResolverVisitor.h
 * @brief Second semantic pass — resolves every name reference to its
 *        defining symbol and attaches concrete type information.
 *
 * SymbolAndTypeResolverVisitor walks the annotated AST produced by
 * SymbolDefinitionVisitor and:
 *   - Resolves each Variable reference to the Symbol that defined it,
 *     emitting an "unknown symbol" error if no definition is found.
 *   - Resolves type-name strings (on immediates, memory operands, etc.)
 *     to their Type objects via the TypeTable.
 *   - Annotates nodes with DataTypeAnnotation and SymbolAnnotation so the
 *     TypeCheckVisitor can validate operand compatibility.
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
 * This resolves every symbol. If a symbol is used but not defined, an error is thrown. It also resolves types
 * used in memory operands.
 */
class SymbolAndTypeResolverVisitor : public SemanticVisitor
{
  public:
    /**
     * Visits given CodeScope node. It will visit its expressions.
     * @param scope
     * @return bool
     */
    bool visit(CodeScope *scope) override;

    /**
     * Visits given CodeScope node. It will visit its expressions.
     * @param cond
     * @return bool
     */
    bool visit(ConditionAstNode *cond) override;

    /**
     * Visits the given IfAstNode. Will try to resolve the symbol and types from the condition and true and false
     * branches.
     * @param ifNode
     * @return bool
     */
    bool visit(IfAstNode *ifNode);

    /**
     * Visits given immediate node. It tries to resolve its type if it was given. If no type was set, the default
     * i64 type is used.
     * @param instr
     * @return bool
     */
    bool visit(ImmediateOperand *imm) override;

    /**
     * Visits given instruction node. If instruction uses virtual variables, this visitor will check if they have been
     * previously defined. If instruction == create, it returns true without doing nothing.
     * @param instr
     * @return bool
     */
    bool visit(Instruction *instr) override;

    /**
     * Visits given Label operand node. Recursively visits sub labels and instructions.
     * @param label
     * @return bool
     */
    bool visit(Label *label) override;

    /**
     * Visits given Memory operand node. If the operand uses virtual variables, this visitor will check if they have
     * been previously defined. It will also create the corresponding type, using POINTER as default (if no type is
     * provided).
     * @param operand
     * @return bool
     */
    bool visit(MemoryOperandAstNode *operand) override;

    /**
     * Visits given Module node. Recursively visits child instructions and labels.
     * @param module
     * @return bool
     */
    bool visit(Module *module) override;

    /**
     * Visits given Variable node. Resolves the symbol this variables represent.
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
};

#endif // EZPACKER_SYMBOLANDTYPERESOLVERVISITOR_H
