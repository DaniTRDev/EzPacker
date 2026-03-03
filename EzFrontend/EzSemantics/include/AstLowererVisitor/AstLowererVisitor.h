#ifndef EZPACKER_ASTLOWERERVISITOR_H
#define EZPACKER_ASTLOWERERVISITOR_H

#include "EzSemanticsCommon.h"
#include "BasicSemanticContext.h"
#include "SemanticVisitor.h"
#include "LoweringContext.h"

// Lowerers are included in the Cpp file to avoid having cyclic dependencies.

class AstLowererVisitor : public SemanticVisitor
{
  public:
    /**
     * Creates the lowerer visitor with the given lowering context.
     * @param loweringCtx
     */
    AstLowererVisitor(const std::shared_ptr<LoweringContext> &loweringCtx);

    /**
     * Visits given CodeScope node. It will visit its expressions.
     * @param scope
     * @return bool
     */
    bool visit(CodeScope *scope) override;

    /**
     * Visits the given condition and emits the corresponding compare and jump to block.
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
     * Visits the given immediate and checks its type.
     * @return
     */
    bool visit(ImmediateOperand *imm) override;

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
     * Visits given Module node. Recursively visits its expressions.
     * @param header
     * @return bool
     */
    bool visit(ModuleHeader *header) override;
    
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
    std::shared_ptr<LoweringContext> m_loweringCtx;
};

#endif // EZPACKER_ASTLOWERERVISITOR_H
