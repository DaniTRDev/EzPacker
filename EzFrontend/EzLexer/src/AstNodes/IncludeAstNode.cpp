#include "AstNodes/IncludeAstNode.h"
#include "AstNode/AstNodeVisitor.h"

IncludeAstNode::IncludeAstNode(const std::string_view &includedFileRelPath) : m_includedFileRelPath(includedFileRelPath)
{
}

AstNodeType IncludeAstNode::getType() const { return AstNodeType::Include; }

bool IncludeAstNode::accept(AstNodeVisitor *visitor)
{
    if (visitor != nullptr)
    {
        return visitor->visit(this);
    }
    return false;
}

const char *IncludeAstNode::getAstNodeName() const { return nullptr; }

const std::string_view &IncludeAstNode::getIncludedFileRelPath() const { return m_includedFileRelPath; }
