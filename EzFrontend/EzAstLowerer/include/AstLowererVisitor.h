/**
 * @file AstLowererVisitor.h
 * @brief Public entry-point visitor for lowering a validated AST into MIR.
 *
 * This visitor is intended to run after the three semantic passes have
 * succeeded. It delegates each supported AST node kind to a specialised
 * lowerer and shares a `AstLoweringContext` across the whole traversal.
 *
 * Pre-conditions for consumers:
 *   - The AST must already carry the semantic annotations produced by the
 *     definition/resolution/type-check pipeline.
 *   - The supplied `AstLoweringContext` must already contain the MIR emitter
 *     services needed by the lowerers.
 *
 * Supported public lowering entry points in this module currently include code
 * scopes, conditions, `if`, immediates, instructions, labels, modules,
 * module headers, memory operands, variables and `while` loops.
 */
#ifndef EZPACKER_ASTLOWERERVISITOR_H
#define EZPACKER_ASTLOWERERVISITOR_H

#include "AstLoweringContext.h"
// Lowerers are included in the Cpp file to avoid having cyclic dependencies.

class AstLowererVisitor : public SemanticVisitor
{
  public:
    /**
     * Creates the lowering visitor that will use the given shared lowering
     * context.
     */
    explicit AstLowererVisitor(const std::shared_ptr<AstLoweringContext> &loweringCtx);

    /**
     * Lowers a code scope by delegating to `CodeScopeLowerer`.
     */
    bool visit(CodeScope *scope) override;

    /**
     * Lowers a condition node into the MIR comparison/jump sequence expected
     * by surrounding control-flow lowerers.
     */
    bool visit(ConditionAstNode *cond) override;

    /**
     * Lowers a for node into the MIR initialization, comparison, body and next iteration clause blocks.
     */
    bool visit(ForAstNode *_for) override;

    /**
     * Lowers an `if` statement by delegating to `IfLowerer`.
     */
    bool visit(IfAstNode *ifNode) override;
    /**
     * Lowers an immediate operand.
     *
     * Any type information required for emission is expected to already be
     * present in semantic annotations.
     */
    bool visit(ImmediateOperand *imm) override;

    /**
     * Lowers one instruction and its operands.
     */
    bool visit(Instruction *instr) override;

    /**
     * Lowers a label and the body associated with it.
     */
    bool visit(Label *label) override;

    /**
     * Lowers a module definition into MIR function/module state.
     */
    bool visit(Module *module) override;

    /**
     * Lowers a module header, typically parameters and signature metadata.
     */
    bool visit(ModuleHeader *header) override;
    
    /**
     * Lowers a switch into its corresponding case condition checker and case body block. Will also handle
     * jumping to the next block if no case is going to be executed and also handles case-fallthrough if no break
     * is specified.
     */
    bool visit(SwitchAstNode *_switch) override;

    /**
     * Lowers a variable use or definition reference.
     *
     * Symbol and cast annotations produced by semantic analysis determine the
     * MIR operand emitted here.
     */
    bool visit(Variable *var) override;

    /**
     * Lowers a `while` loop by delegating to `WhileLowerer`.
     */
    bool visit(WhileAstNode *whileNode) override;

  private:
    std::shared_ptr<AstLoweringContext> m_loweringCtx;
};

#endif // EZPACKER_ASTLOWERERVISITOR_H
