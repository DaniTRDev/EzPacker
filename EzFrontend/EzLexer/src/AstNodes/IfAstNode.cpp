#include "AstNodes/IfAstNode.h"

AstNode *IfAstNode::getFalseScope() const { return m_falseScope; }

AstNodeType IfAstNode::getType() const { return AstNodeType::If; }

bool IfAstNode::accept(AstNodeVisitor *visitor)
{
    if (visitor)
    {
        return visitor->visit(this);
    }
    return false;
}

CodeScope *IfAstNode::getTrueScope() const { return m_trueScope; }

ConditionAstNode *IfAstNode::getCondition() const { return m_condition; }

const char *IfAstNode::getAstNodeName() const { return "IfAstNode"; }

void IfAstNode::setFalseScope(AstNode *falseScope) { m_falseScope = falseScope; }

void IfAstNode::setTrueScope(CodeScope *trueScope) { m_trueScope = trueScope; }

void IfAstNode::setCondition(ConditionAstNode *condition) { m_condition = condition; }
