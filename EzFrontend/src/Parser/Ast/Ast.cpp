#include "parser/ast/Ast.h"

Ast::~Ast()
{
    m_children.clear();
}

AstType Ast::getType() const
{
    return m_type;
}

bool Ast::hasChildren() const
{
    return m_children.size();
}

void Ast::addChild(std::shared_ptr<Ast> node)
{
    m_children.push_back(node);
}

void Ast::clearChildren()
{
    m_children.clear();
}

void Ast::copyChildrenTo(const std::shared_ptr<Ast> &other)
{
    for(auto &child : m_children)
    {
        other->addChild(child);
    }
}

void Ast::setType(AstType type)
{
    m_type = type;
}

const std::vector<std::shared_ptr<Ast>> &Ast::getChildren()
{
    return m_children;
}
