#include "AstNodes/WhileAstNode.h"

WhileAstNode::WhileAstNode() {}

AstNodeType WhileAstNode::getType() const { return AstNodeType::While; }

const char *WhileAstNode::getAstNodeName() const { return "WhileAstNode"; }

void WhileAstNode::setCodeScope(const std::shared_ptr<CodeScope> &codeScope) { m_codeScope = codeScope; }

void WhileAstNode::setCondition(const std::shared_ptr<ConditionAstNode> &condition) { m_condition = condition; }

const std::shared_ptr<ConditionAstNode> &WhileAstNode::getCondition() const { return m_condition; }

const std::shared_ptr<CodeScope> &WhileAstNode::getCodeScope() const { return m_codeScope; }

std::string WhileAstNode::getAsStr(AstNodeStringMode mode) const
{
    std::string str = std::format("while ({})\n{{\n{}\n}}\n", m_condition->getAsStr(mode), m_codeScope->getAsStr(mode));
    return str;
}
