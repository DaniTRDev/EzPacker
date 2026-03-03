#include "AstNodes/WhileAstNode.h"

WhileAstNode::WhileAstNode() {}

AstNodeType WhileAstNode::getType() const { return AstNodeType::While; }

bool WhileAstNode::accept(AstNodeVisitor *visitor)
{
    if (visitor)
    {
        return visitor->visit(this);
    }
    return false;
}

CodeScope *WhileAstNode::getCodeScope() const { return m_codeScope; }

ConditionAstNode *WhileAstNode::getCondition() const { return m_condition; }

const char *WhileAstNode::getAstNodeName() const { return "WhileAstNode"; }

void WhileAstNode::setCodeScope(CodeScope *codeScope) { m_codeScope = codeScope; }

void WhileAstNode::setCondition(ConditionAstNode *condition) { m_condition = condition; }

std::string WhileAstNode::getAsStr(AstNodeStringMode mode) const
{
    std::string str = std::format("while ({})\n{{\n{}\n}}\n", m_condition->getAsStr(mode), m_codeScope->getAsStr(mode));
    return str;
}
