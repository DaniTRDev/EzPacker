/**
 * @file ConditionAstNode.h
 * @brief AST node for a binary comparison expression used in `if` and `while`.
 *
 * A ConditionAstNode holds a left operand, a right operand, and a
 * ConditionComparisonType (EQ, NE, GT, GE, LT, LE).  During lowering the
 * ConditionLowerer translates this into a CMP instruction followed by the
 * appropriate conditional jump.
 */
#ifndef EZPACKER_CONDITIONASTNODE_H
#define EZPACKER_CONDITIONASTNODE_H

#include "EzLexerCommon.h"
#include "AstNode/AstNode.h"
#include "AstNode/AstNodeVisitor.h"

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
     * @param comparisonType
     * @param left
     * @param right
     */
    ConditionAstNode(AstNode *left, AstNode *right, ConditionComparisonType comparisonType);

    /**
     * Returns the left-hand side of the condition (e.g., a variable or expression).
     * @return AstNode *
     */
    AstNode *getLeft() const;

    /**
     * Returns the right-hand side of the condition (e.g., a variable, expression, or literal).
     * @return AstNode *
     */
    AstNode *getRight() const;

    /**
     * Returns 'Condition' as the type of this node.
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
     * Returns the type of condition (e.g., Equal, GreaterThan, etc.).
     * @return ComparisonType
     */
    ConditionComparisonType getComparisonType() const;

    /**
     * Returns "ConditionAstNode".
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Returns a string representation of this node. This is described as:
     * op1 comparison_operator op2
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;

  private:
    ConditionComparisonType m_comparisonType; // The type of condition (e.g., Equal, GreaterThan, etc.)
    AstNode *m_left;                          // The left-hand side of the condition (e.g., a variable or expression).
    AstNode *m_right; // The right-hand side of the condition (e.g., a variable, expression, or literal).
};

#endif // EZPACKER_CONDITIONASTNODE_H
