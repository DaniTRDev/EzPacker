#ifndef EZPACKER_IFASTNODE_H
#define EZPACKER_IFASTNODE_H

#include "EzLexerCommon.h"
#include "CodeScope.h"
#include "ConditionAstNode.h"
#include "AstNode/AstNode.h"

class IfAstNode : public AstNode
{
  public:
    /**
     * Returns 'If' as the type of this node.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

    /**
     * Returns 'IfAstNode'.
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Sets the false scope of this if statement, which is the code to execute if the condition evaluates to false.
     * @param falseScope
     */
    void setFalseScope(const std::shared_ptr<AstNode> &falseScope);

    /**
     * Sets the true scope of this if statement, which is the code to execute if the condition evaluates to true.
     * @param trueScope
     */
    void setTrueScope(const std::shared_ptr<CodeScope> &trueScope);

    /**
     * Sets the condition of this node.
     * @param condition
     */
    void setCondition(const std::shared_ptr<ConditionAstNode> &condition);

    /**
     * Gets the false scope of this if statement, which can be either an else block or an else-if block.
     * @return std::shared_ptr<AstNode>
     */
    const std::shared_ptr<AstNode> &getFalseScope() const;

    /**
     * Gets the true scope of this if statement.
     * @return std::shared_ptr<CodeScope>
     */
    const std::shared_ptr<CodeScope> &getTrueScope() const;

    /**
     * Gets the condition of this if statement.
     */
    const std::shared_ptr<ConditionAstNode> &getCondition() const;

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
    std::shared_ptr<AstNode> m_falseScope;  /** Node that handles the else part of the if statement, if it exists.
                                             * It can either be a CodeScope for an else block or another IfAstNode for an
                                             * else-if block. If there is no else or else-if, this will be nullptr.
                                             */
    std::shared_ptr<CodeScope> m_trueScope; // Code to execute if the condition is true.
    std::shared_ptr<ConditionAstNode> m_condition; // The condition to evaluate for this if statement.
};

#endif // EZPACKER_IFASTNODE_H
