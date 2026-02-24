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
#include "AstNodes/IfAstNode.h"
#include "AstNodes/ConditionAstNode.h"
#include "AstNodes/WhileAstNode.h"

class AstNodeVisitor
{
  public:
    virtual ~AstNodeVisitor() = default;

    /**
     * Visits given CodeScope by visiting its expressions.
     * @param scope
     * @return bool
     */
    virtual bool visit(const std::shared_ptr<CodeScope> &scope)
    {
        return visitAll(scope->getExpressions(), [](const auto &expr) { return expr.second; });
    }

    /**
     * Visits given instruction node and visits its operands. Should return true visitor wants to keep traversing the
     * tree.
     * @param instr
     * @return bool
     */
    virtual bool visit(const std::shared_ptr<IfAstNode> &ifNode)
    {
        return visit(ifNode->getTrueScope()) && (!ifNode->getFalseScope() || visitBaseClass(ifNode->getFalseScope()));
    }

    /**
     * Visits given Immediate operand node. Should return true visitor wants to keep traversing the tree.
     * @param operand
     * @return bool
     */
    virtual bool visit(const std::shared_ptr<ImmediateOperand> &operand) { return true; }

    /**
     * Visits given instruction node and visits its operands. Should return true visitor wants to keep traversing the
     * tree.
     * @param instr
     * @return bool
     */
    virtual bool visit(const std::shared_ptr<Instruction> &instr) { return visitAll(instr->getOperands()); }

    /**
     * Visits given Label operand node and visits its code scope. Should return true visitor wants to keep traversing
     * the tree.
     * @param label
     * @return
     */
    virtual bool visit(const std::shared_ptr<Label> &label) { return visit(label->getCodeScope()); }

    /**
     * Visits given Memory operand node. Should return true visitor wants to keep traversing the tree.
     * @param operand
     * @return bool
     */
    virtual bool visit(const std::shared_ptr<MemoryOperandAstNode> &operand) { return true; }

    /**
     * Visits given Module node and visits its body scope. Should return true visitor wants to keep traversing the tree.
     * @param module
     * @return bool
     */
    virtual bool visit(const std::shared_ptr<Module> &module) { return visit(module->getBody()); }

    /**
     * Visits given Variable node. Should return true visitor wants to keep traversing the tree.
     * @param var
     * @return bool
     */
    virtual bool visit(const std::shared_ptr<Variable> &var) { return true; }

    /**
     * Visits given While node and visits its code scope. Should return true visitor wants to keep traversing the tree.
     * @param var
     * @return
     */
    virtual bool visit(const std::shared_ptr<WhileAstNode> &whileNode) { return visit(whileNode->getCodeScope()); }

    /**
     * Traversers every node in this AstNodeContainer. Returns true if succeeded.
     * @tparam Container
     * @param items
     * @return bool
     */
    bool visitAll(const std::vector<std::shared_ptr<AstNode>> &nodes);

    /**
     * Iterates over the given container of structures that can have an AstNode, the getter is called to retrieve
     * the AstNode from the element of the container. Returns true if succeeded.
     * @tparam Container
     * @tparam Getter
     * @param container
     * @param getter
     * @return
     */
    template <typename Container, typename Getter> bool visitAll(const Container &container, const Getter &getter)
    {
        for (auto &elem : container)
        {
            auto node = std::dynamic_pointer_cast<AstNode>(getter(elem));
            if (!visitBaseClass(node))
            {
                return false;
            }
        }
        return true;
    }

  protected:
    /**
     * Resolves the real node type and calls its appropriated visitor. Returns true if succeeded.
     * @param astNode
     * @return bool
     */
    bool visitBaseClass(const std::shared_ptr<AstNode> &astNode);
};

#endif // EZPACKER_ASTNODEVISITOR_H
