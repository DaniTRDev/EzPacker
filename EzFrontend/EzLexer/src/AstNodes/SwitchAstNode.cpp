#include "AstNodes/SwitchAstNode.h"

SwitchAstNode::SwitchAstNode(Variable *switchVariable) :
    m_default(nullptr), m_cases(nullptr), m_switchVariable(switchVariable)
{
}

AstNodeType SwitchAstNode::getType() const { return AstNodeType::Switch; }

bool SwitchAstNode::accept(AstNodeVisitor *visitor)
{
    if (visitor)
    {
        return visitor->visit(this);
    }
    return false;
}

const char *SwitchAstNode::getAstNodeName() const { return "SwitchAstNode"; }

CodeScope *SwitchAstNode::getDefault() { return m_default; }

TypedPoolSlice<AstNode> *SwitchAstNode::getCases() { return m_cases; }

Variable *SwitchAstNode::getSwitchVariable() { return m_switchVariable; }

void SwitchAstNode::setCases(TypedPoolSlice<AstNode> *cases) { m_cases = cases; }

void SwitchAstNode::setDefault(CodeScope *_default) { m_default = _default; }
