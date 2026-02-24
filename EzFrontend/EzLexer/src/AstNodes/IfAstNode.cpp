#include "AstNodes/IfAstNode.h"

AstNodeType IfAstNode::getType() const { return AstNodeType::If; }

const char *IfAstNode::getAstNodeName() const { return "IfAstNode"; }

void IfAstNode::setFalseScope(const std::shared_ptr<AstNode> &falseScope) { m_falseScope = falseScope; }

void IfAstNode::setTrueScope(const std::shared_ptr<CodeScope> &trueScope) { m_trueScope = trueScope; }

void IfAstNode::setCondition(const std::shared_ptr<ConditionAstNode> &condition) { m_condition = condition; }

const std::shared_ptr<AstNode> &IfAstNode::getFalseScope() const { return m_falseScope; }

const std::shared_ptr<CodeScope> &IfAstNode::getTrueScope() const { return m_trueScope; }

const std::shared_ptr<ConditionAstNode> &IfAstNode::getCondition() const { return m_condition; }

std::string IfAstNode::getAsStr(AstNodeStringMode mode) const
{
    std::string str = std::format("if ({})\n{{\n{}\n}}\n", m_condition->getAsStr(mode), m_trueScope->getAsStr(mode));

    if (!m_falseScope)
        return str;

    if (m_falseScope->getType() == AstNodeType::If)
    {
        str += std::format("elif\n{{\n{}\n}}", m_falseScope->getAsStr(mode));
    }
    else
    {
        str += std::format("else\n{{\n{}\n}}", m_falseScope->getAsStr(mode));
    }

    return str;
}
