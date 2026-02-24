#include "AstNodes/ConditionAstNode.h"

ConditionAstNode::ConditionAstNode(ConditionComparisonType comparisonType,
                                   const std::shared_ptr<AstNode> &left,
                                   const std::shared_ptr<AstNode> &right) :
    m_comparisonType(comparisonType), m_left(left), m_right(right)
{
}

AstNodeType ConditionAstNode::getType() const { return AstNodeType::Condition; }

ConditionComparisonType ConditionAstNode::getComparisonType() const { return m_comparisonType; }

const char *ConditionAstNode::getAstNodeName() const { return "ConditionAstNode"; }

const std::shared_ptr<AstNode> &ConditionAstNode::getLeft() const { return m_left; }

const std::shared_ptr<AstNode> &ConditionAstNode::getRight() const { return m_right; }

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
