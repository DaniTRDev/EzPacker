#ifndef EZPACKER_IFASTNODE_H
#define EZPACKER_IFASTNODE_H

#include "EzLexerCommon.h"
#include "CodeScope.h"
#include "ConditionAstNode.h"
#include "AstNode/AstNode.h"
#include "AstNode/AstNodeVisitor.h"

class IfAstNode : public AstNode
{
  public:
    /**
     * Gets the false scope of this if statement, which can be either an else block or an else-if block.
     * @return AstNode *
     */
    AstNode *getFalseScope() const;

    /**
     * Returns 'If' as the type of this node.
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
     * Gets the true scope of this if statement.
     * @return CodeScope *
     */
    CodeScope *getTrueScope() const;

    /**
     * Gets the condition of this if statement.
     * @return ConditionAstNode *
     */
    ConditionAstNode *getCondition() const;

    /**
     * Returns 'IfAstNode'.
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Sets the false scope of this if statement, which is the code to execute if the condition evaluates to false.
     * @param falseScope
     */
    void setFalseScope(AstNode *falseScope);

    /**
     * Sets the true scope of this if statement, which is the code to execute if the condition evaluates to true.
     * @param trueScope
     */
    void setTrueScope(CodeScope *trueScope);

    /**
     * Sets the condition of this node.
     * @param condition
     */
    void setCondition(ConditionAstNode *condition);

    /**
     * Returns a string representation of this node. This is described as:
     * if (condition)
     * {CodeScope::getAsStr(Default)}
     * else || elif ...
     * {}
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;

  private:
    AstNode *m_falseScope;         /** Node that handles the else part of the if statement, if it exists.
                                    * It can either be a CodeScope for an else block or another IfAstNode for an
                                    * else-if block. If there is no else or else-if, this will be nullptr.
                                    */
    CodeScope *m_trueScope;        // Code to execute if the condition is true.
    ConditionAstNode *m_condition; // The condition to evaluate for this if statement.
};

#endif // EZPACKER_IFASTNODE_H
