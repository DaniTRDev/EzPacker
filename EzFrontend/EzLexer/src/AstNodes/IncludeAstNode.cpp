#include "AstNodes/IncludeAstNode.h"
#include "AstNode/AstNodeVisitor.h"

AstNodeType IncludeAstNode::getType() const { return AstNodeType::Include; }

bool IncludeAstNode::accept(AstNodeVisitor *visitor)
{
    if (visitor != nullptr)
    {
        visitor->visit(this);
    }
    return false;
}

const char *IncludeAstNode::getAstNodeName() const { return nullptr; }

std::string IncludeAstNode::getAsStr(AstNodeStringMode mode) const
{
    return std::format("include <{}>", m_includedFileRelPath);
}

const std::string_view &IncludeAstNode::getIncludedFileRelPath() const { return m_includedFileRelPath; }
