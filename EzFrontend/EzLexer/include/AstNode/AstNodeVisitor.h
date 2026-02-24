#ifndef EZPACKER_ASTNODEVISITOR_H
#define EZPACKER_ASTNODEVISITOR_H

#include "EzLexerCommon.h"
#include "AstNode.h"
#include "AstNodes/ImmediateOperand.h"
#include "AstNodes/Instruction.h"
#include "AstNodes/Label.h"
#include "AstNodes/MemoryOperand.h"
#include "AstNodes/Module.h"
#include "AstNodes/Variable.h"

class AstNodeVisitor
{
  public:
    virtual ~AstNodeVisitor() = default;

    /**
     * Visits given CodeScope by visiting its expressions.
     * @param module
     * @return bool
     */
    virtual bool visit(const std::shared_ptr<struct CodeScope> &code) { return true; }

    /**
     * Visits given Immediate operand node. Should return true visitor wants to keep traversing the tree.
     * @param operand
     * @return bool
     */
    virtual bool visit(const std::shared_ptr<struct ImmediateOperand> &operand) { return true; }

    /**
     * Visits given instruction node. Should return true visitor wants to keep traversing the tree.
     * @param instr
     * @return bool
     */
    virtual bool visit(const std::shared_ptr<struct Instruction> &instr) { return true; }

    /**
     * Visits given Label operand node. Should return true visitor wants to keep traversing the tree.
     * @param label
     * @return
     */
    virtual bool visit(const std::shared_ptr<struct Label> &label) { return true; }

    /**
     * Visits given Memory operand node. Should return true visitor wants to keep traversing the tree.
     * @param operand
     * @return bool
     */
    virtual bool visit(const std::shared_ptr<struct MemoryOperandAstNode> &operand) { return true; }

    /**
     * Visits given Module node. Should return true visitor wants to keep traversing the tree.
     * @param module
     * @return bool
     */
    virtual bool visit(const std::shared_ptr<struct Module> &module) { return true; }

    /**
     * Visits given Variable node. Should return true visitor wants to keep traversing the tree.
     * @param var
     * @return bool
     */
    virtual bool visit(const std::shared_ptr<struct Variable> &var) { return true; }

  protected:
    /**
     * Resolves the real node type and calls its appropriated visitor. Returns true if succeeded.
     * @param astNode
     * @return bool
     */
    bool visitBaseClass(const std::shared_ptr<struct AstNode> &astNode);
};

#endif // EZPACKER_ASTNODEVISITOR_H
