/**
 * @file WhileAstNode.h
 * @brief AST node for `while` loops: `while (condition) { body }`.
 *
 * A WhileAstNode owns a ConditionAstNode and a CodeScope.  During lowering,
 * the WhileLowerer creates three basic blocks (condition-check, loop-body,
 * exit) and pushes a LoopContext so that any break/continue statements
 * inside the body know which blocks to target.
 */
#ifndef EZPACKER_WHILEASTNODE_H
#define EZPACKER_WHILEASTNODE_H

#include "EzLexerCommon.h"
#include "AstNode/AstNode.h"
#include "AstNode/AstNodeVisitor.h"
#include "AstNodes/CodeScope.h"
#include "AstNodes/ConditionAstNode.h"

class WhileAstNode : public AstNode
{
  public:
    /**
     * Creates the node.
     */
    WhileAstNode();

    /**
     * Returns 'While' as the type of this node.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

    /**
     * Accepts the given visitor and calls its internal visit method with the correct node type. Returns
     * the result of visit.
     * @param visitor
     * @return bool
     */
    bool accept(AstNodeVisitor *visitor) override;
    
    /**
     * Gets the code scope of this while loop.
     * @return CodeScope *
     */
    CodeScope *getCodeScope() const;

    /**
     * Gets the condition of this while loop.
     * @return ConditionAstNode *
     */
    ConditionAstNode *getCondition() const;

    /**
     * Returns 'WhileAstNode'.
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Sets the code scope for this while loop.
     * @param codeScope
     */
    void setCodeScope(CodeScope *codeScope);

    /**
     * Sets the condition for this while loop.
     * @param condition
     */
    void setCondition(ConditionAstNode *condition);

  private:
    ConditionAstNode *m_condition;
    CodeScope *m_codeScope;
};

#endif // EZPACKER_WHILEASTNODE_H
