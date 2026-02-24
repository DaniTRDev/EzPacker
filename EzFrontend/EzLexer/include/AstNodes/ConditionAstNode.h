#ifndef EZPACKER_CONDITIONASTNODE_H
#define EZPACKER_CONDITIONASTNODE_H

#include "EzLexerCommon.h"
#include "AstNode/AstNode.h"

enum class ConditionComparisonType
{
    Equal,
    GreaterThan,
    GreaterThanOrEqual,
    LessThan,
    LessThanOrEqual,
    NotEqual
};

class ConditionAstNode : public AstNode
{
  public:
    /**
     * Creates the condition node with the specified type and left and right operands.
     * @param conditionType
     * @param left
     * @param right
     */
    ConditionAstNode(ConditionComparisonType conditionType,
                     const std::shared_ptr<AstNode> &left,
                     const std::shared_ptr<AstNode> &right);

    /**
     * Returns 'Condition' as the type of this node.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

    /**
     * Returns the type of condition (e.g., Equal, GreaterThan, etc.).
     * @return ComparisonType
     */
    ConditionComparisonType getComparisonType() const;

    /**
     * Returns 'Condition'.
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Returns the left-hand side of the condition (e.g., a variable or expression).
     * @return const std::shared_ptr<AstNode> &
     */
    const std::shared_ptr<AstNode> &getLeft() const;

    /**
     * Returns the right-hand side of the condition (e.g., a variable, expression, or literal).
     * @return const std::shared_ptr<AstNode> &
     */
    const std::shared_ptr<AstNode> &getRight() const;

    /**
     * Returns a string representation of this node. This is described as:
     * op1 comparison_operator op2
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;

  private:
    ConditionComparisonType m_comparisonType; // The type of condition (e.g., Equal, GreaterThan, etc.)
    std::shared_ptr<AstNode> m_left;          // The left-hand side of the condition (e.g., a variable or expression).
    std::shared_ptr<AstNode>
            m_right; // The right-hand side of the condition (e.g., a variable, expression, or literal).
};

#endif // EZPACKER_CONDITIONASTNODE_H
