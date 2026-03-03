#include "AstNodes/BreakAstNode.h"
#include "AstNode/AstNodeVisitor.h"

BreakAstNode::BreakAstNode() {}

AstNodeType BreakAstNode::getType() const { return AstNodeType::Break; }

bool BreakAstNode::accept(struct AstNodeVisitor *visitor)
{
    if (visitor)
    {
        return visitor->visit(this);
    }
    return false;
}

const char *BreakAstNode::getAstNodeName() const { return "BreakAstNode"; }

std::string BreakAstNode::getAsStr(AstNodeStringMode mode) const { return getAstNodeName(); }
