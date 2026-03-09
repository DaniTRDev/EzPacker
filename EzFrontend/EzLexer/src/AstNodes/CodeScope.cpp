#include "AstNodes/CodeScope.h"

AstNodeType CodeScope::getType() const { return AstNodeType::CodeScope; }

bool CodeScope::accept(AstNodeVisitor *visitor)
{
    if (visitor)
    {
        return visitor->visit(this);
    }

    return false;
}

const char *CodeScope::getAstNodeName() const { return "CodeScope"; }
