#ifndef EZPACKER_ASTNODEVISITOR_H
#define EZPACKER_ASTNODEVISITOR_H

#include "EzLexerCommon.h"
#include "AstNode.h"
/**
 * Base class for the Visitor pattern over the AST. Subclasses override individual visit() methods
 * to perform operations on specific node types. Each visit() returns true to continue traversal
 * or false to stop. The default implementation of every visit() returns true (no-op).
 *
 * Use visitAll() to iterate an entire TypedPoolSlice of nodes, or visitBaseClass() to dispatch
 * a single AstNode* to its concrete visit() overload.
 */
class AstNodeVisitor
{
  public:
    virtual ~AstNodeVisitor() = default;

    /**
     * Visits given ContinueAstNode.
     * @param _break
     * @return bool
     */
    virtual bool visit(class BreakAstNode *_break) { return true; }
    
    /**
     * Visits given CodeScope by visiting its expressions.
     * @param scope
     * @return bool
     */
    virtual bool visit(class CodeScope *scope) { return true; }
    
    /**
     * Visits given ConditionAstNode by visiting its condition nodes, true case and false case.
     * @param condition
     * @return bool
     */
    virtual bool visit(class ConditionAstNode *condition) { return true; }
    
    /**
     * Visits given ContinueAstNode.
     * @param _continue
     * @return bool
     */
    virtual bool visit(class ContinueAstNode *_continue) { return true; }

    /**
     * Visits given instruction node and visits its operands. Should return true visitor wants to keep traversing the
     * tree.
     * @param instr
     * @return bool
     */
    virtual bool visit(class IfAstNode *ifNode) { return true; }

    /**
     * Visits given Immediate operand node. Should return true visitor wants to keep traversing the tree.
     * @param operand
     * @return bool
     */
    virtual bool visit(class ImmediateOperand *operand) { return true; }

    /**
     * Visits given instruction node and visits its operands. Should return true visitor wants to keep traversing the
     * tree.
     * @param instr
     * @return bool
     */
    virtual bool visit(class Instruction *instr) { return true; }

    /**
     * Visits given Label operand node and visits its code scope. Should return true visitor wants to keep traversing
     * the tree.
     * @param label
     * @return
     */
    virtual bool visit(class Label *label) { return true; }

    /**
     * Visits given Memory operand node. Should return true visitor wants to keep traversing the tree.
     * @param operand
     * @return bool
     */
    virtual bool visit(class MemoryOperandAstNode *operand) { return true; }

    /**
     * Visits given Module node and visits its header. Should return true visitor wants to keep traversing the tree.
     * @param module
     * @return bool
     */
    virtual bool visit(class ModuleHeader *module) { return true; }

    /**
     * Visits given Module node and visits its body scope and header. Should return true visitor wants to keep
     * traversing the tree.
     * @param module
     * @return bool
     */
    virtual bool visit(class Module *module) { return true; }

    /**
     * Visits given Variable node. Should return true visitor wants to keep traversing the tree.
     * @param var
     * @return bool
     */
    virtual bool visit(class Variable *var) { return true; }

    /**
     * Visits given While node and visits its code scope. Should return true visitor wants to keep traversing the tree.
     * @param var
     * @return
     */
    virtual bool visit(class WhileAstNode *whileNode) { return true; }

    /**
     * Traversers every node in this AstNodeContainer. Returns true if succeeded.
     * @param nodeList
     * @return bool
     */
    bool visitAll(TypedPoolSlice<AstNode> *nodeList);

  protected:
    /**
     * Resolves the real node type and calls its appropriated visitor. Returns true if succeeded.
     * @param astNode
     * @return bool
     */
    bool visitBaseClass(AstNode *astNode);
};

#endif // EZPACKER_ASTNODEVISITOR_H
