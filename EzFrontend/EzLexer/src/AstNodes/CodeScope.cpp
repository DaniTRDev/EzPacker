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

std::string CodeScope::getAsStr(AstNodeStringMode mode) const
{
    TypedPoolSlice<AstNode> *expressions = getExpressions();
    if (mode == AstNodeStringMode::Debug)
    {
        std::string str = "CodeScope\n{\n";

        for (void *expr : *expressions)
        {
            AstNode *node = (AstNode *)expr;
            str += node->getAsStr(mode);
        }

        return str + "\n}\n";
    }

    return std::format("CodeScope (size: {})", expressions->m_numElems);
}
