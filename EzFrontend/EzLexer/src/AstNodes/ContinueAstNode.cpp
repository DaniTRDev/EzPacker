#include "AstNodes/ContinueAstNode.h"
#include "AstNode/AstNodeVisitor.h"

ContinueAstNode::ContinueAstNode() {}

AstNodeType ContinueAstNode::getType() const { return AstNodeType::Continue; }

bool ContinueAstNode::accept(struct AstNodeVisitor *visitor)
{
    if (visitor)
    {
        return visitor->visit(this);
    }
    return false;
}

const char *ContinueAstNode::getAstNodeName() const { return "ContinueAstNode"; }