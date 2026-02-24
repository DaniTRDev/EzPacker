#ifndef EZPACKER_MODULEINSTRUCTIONLOWERERVISITOR_H
#define EZPACKER_MODULEINSTRUCTIONLOWERERVISITOR_H

#include "EzSemanticsCommon.h"
#include "BasicSemanticContext.h"
#include "SemanticVisitor.h"
#include "HighLevelMir/HighLevelMirModule.h"
#include "SemanticAnnotations/DataTypeAnnotation.h"
#include "SemanticAnnotations/ScopedSymbolAnnotation.h"
#include "SemanticAnnotations/TypeCastAnnotation.h"

/**
 * Visitor responsible of lowering the AST to a flattened High-Level MIR within a MODULE scope.
 * IMPORTANT: Caller must ensure that there's A SINGLE call to visit with a module node.
 */
class ModuleInstructionLowererVisitor : public SemanticVisitor
{
  public:
    /**
     * Creates the visitor.
     * @param module
     */
    ModuleInstructionLowererVisitor(const std::shared_ptr<HighLevelMirModule> &module);

    /**
     * Lowers the body of a scope (from labels or modules).
     * @param node
     * @param expressionContainer
     * @return bool
     */
    bool visit(const std::shared_ptr<struct CodeScope> &scope) override;

    /**
     * Annotates this node with a HighLevelMirAnnotation containing information about the instruction.
     * @param instr
     * @return bool
     */
    bool visit(const std::shared_ptr<struct Instruction> &instr) override;

    /**
     * Iterates over all the instructions inside the label.
     * @param label
     * @return bool
     */
    bool visit(const std::shared_ptr<struct Label> &label) override;

    /**
     * Iterates over all the instructions inside the module.
     * @param module
     * @return bool
     */
    bool visit(const std::shared_ptr<struct Module> &module) override;

    /**
     * Returns the current block being built.
     * @return std::shared_ptr<HighLevelMirBlock>
     */
    std::shared_ptr<HighLevelMirBlock> getCurrentBlock();
    /**
     * Returns the root block (the first block created).
     * @return std::shared_ptr<HighLevelMirBlock>
     */
    std::shared_ptr<HighLevelMirBlock> getModuleBlock() const;

  private:
    /**
     * Tries to lower the given node, into MIR by calling one of the functions below. If succeeded, inserts the
     * resulting operand into the given instruction and returns true. Returns false other ways.
     * @param instruction
     * @param node
     * @return bool
     */
    bool lowerOperand(HighLevelMirInstruction &instruction, const std::shared_ptr<AstNode> &node);

    /**
     * Tries to lower the given node, which is an immediate, into MIR. If succeeded, inserts the resulting operand into
     * the given instruction and returns true. Returns false other ways.
     * @param instruction
     * @param node
     * @return bool
     */
    bool lowerImmediateOperand(HighLevelMirInstruction &instruction, const std::shared_ptr<AstNode> &node);

    /**
     * Tries to lower the given node, which is a memory operand, into MIR. If succeeded, inserts the resulting operand
     * into the given instruction and returns true. Returns false other ways.
     * @param instruction
     * @param node
     * @return bool
     */
    bool lowerMemoryOperand(HighLevelMirInstruction &instruction, const std::shared_ptr<AstNode> &node);

    /**
     * Tries to lower the given node, which is a variable, into MIR. If succeeded, inserts the resulting operand into
     * the given instruction and returns true. Returns false other ways.
     * @param instruction
     * @param node
     * @return bool
     */
    bool lowerVariableOperand(HighLevelMirInstruction &instruction, const std::shared_ptr<AstNode> &node);

  private:
    std::shared_ptr<HighLevelMirModule> m_moduleBlock;
    std::shared_ptr<HighLevelMirBlock> m_currentBlock;
};

#endif // EZPACKER_MODULEINSTRUCTIONLOWERERVISITOR_H
