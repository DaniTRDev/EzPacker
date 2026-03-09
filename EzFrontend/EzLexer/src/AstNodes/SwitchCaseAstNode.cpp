#include "AstNodes/SwitchCaseAstNode.h"

SwitchCaseAstNode::SwitchCaseAstNode(CodeScope *caseScope, ImmediateOperand *caseValue) :
    m_caseBody(caseScope), m_caseValue(caseValue)
{
}

AstNodeType SwitchCaseAstNode::getType() const { return AstNodeType::SwitchCase; }

bool SwitchCaseAstNode::accept(AstNodeVisitor *visitor)
{
    if (visitor)
    {
        return visitor->visit(this);
    }
    return false;
}

const char *SwitchCaseAstNode::getAstNodeName() const { return "SwitchCastAstNode"; }

CodeScope *SwitchCaseAstNode::getBody() { return m_caseBody; }

ImmediateOperand *SwitchCaseAstNode::getCaseValue() { return m_caseValue; }