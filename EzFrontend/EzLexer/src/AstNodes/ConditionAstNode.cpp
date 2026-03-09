#include "AstNodes/ConditionAstNode.h"

ConditionAstNode::ConditionAstNode(AstNode *left, AstNode *right, ConditionComparisonType comparisonType) :
    m_left(left), m_right(right), m_comparisonType(comparisonType)
{
}

AstNode *ConditionAstNode::getLeft() const { return m_left; }

AstNode *ConditionAstNode::getRight() const { return m_right; }

AstNodeType ConditionAstNode::getType() const { return AstNodeType::Condition; }

bool ConditionAstNode::accept(AstNodeVisitor *visitor)
{
    if (visitor)
    {
        return visitor->visit(this);
    }

    return false;
}

ConditionComparisonType ConditionAstNode::getComparisonType() const { return m_comparisonType; }

const char *ConditionAstNode::getAstNodeName() const { return "ConditionAstNode"; }