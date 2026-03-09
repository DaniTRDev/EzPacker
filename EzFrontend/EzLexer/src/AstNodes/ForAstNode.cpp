#include "AstNodes/ForAstNode.h"

ForAstNode::ForAstNode(CodeScope *body, CodeScope *initialization, CodeScope *nextIt, ConditionAstNode *condition) :
    m_body(body), m_initialization(initialization), m_nextIt(nextIt), m_condition(condition)
{
}

AstNodeType ForAstNode::getType() const { return AstNodeType::For; }

bool ForAstNode::accept(AstNodeVisitor *visitor)
{
    if (visitor)
    {
        return visitor->visit(this);
    }
    return false;
}

const char *ForAstNode::getAstNodeName() const { return "ForAstNode"; }

CodeScope *ForAstNode::getBody() const { return m_body; }

CodeScope *ForAstNode::getInitialization() const { return m_initialization; }

CodeScope *ForAstNode::getNextItClause() const { return m_nextIt; }

ConditionAstNode *ForAstNode::getCondition() const { return m_condition; }
