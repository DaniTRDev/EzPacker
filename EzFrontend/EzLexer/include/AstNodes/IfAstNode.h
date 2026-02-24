#ifndef EZPACKER_IFASTNODE_H
#define EZPACKER_IFASTNODE_H

#include "AstNode/AstNode.h"
#include "CodeScope.h"

enum class IfConditionType
{
    Equal,
    GreaterThan,
    GreaterThanOrEqual,
    LessThan,
    LessThanOrEqual,
    NotEqual
};

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

  private:
    IfConditionType m_conditionType;       // The type of condition for the if statement.
    std::shared_ptr<AstNode> m_operand1;   // First operand of the condition.
    std::shared_ptr<AstNode> m_operand2;   // Second operand of the condition.
    std::shared_ptr<AstNode> m_falseScope; /** Node that handles the else part of the if statement, if it exists.
                                            * It can either be a CodeScope for an else block or another IfAstNode for an
                                            * else-if block. If there is no else or else-if, this will be nullptr.
                                            */

    std::shared_ptr<CodeScope> m_trueScope; // Code to execute if the condition is true.
};

#endif // EZPACKER_IFASTNODE_H
