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

std::string ConditionAstNode::getAsStr(AstNodeStringMode mode) const
{
    std::string str = m_left->getAsStr(mode) + " ";
    switch (m_comparisonType)
    {
        case ConditionComparisonType::Equal:
        {
            str += "==";
            break;
        }
        case ConditionComparisonType::NotEqual:
        {
            str += "!=";
            break;
        }
        case ConditionComparisonType::GreaterThan:
        {
            str += ">";
            break;
        }
        case ConditionComparisonType::LessThan:
        {
            str += "<";
            break;
        }
        case ConditionComparisonType::GreaterThanOrEqual:
        {
            str += ">=";
            break;
        }
        case ConditionComparisonType::LessThanOrEqual:
        {
            str += "<=";
            break;
        }
    }
    str += " " + m_right->getAsStr(mode);
    return str;
}
