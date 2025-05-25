#include "parser/ast/Ast.h"

Ast::Ast() : m_mergeSourceReferences(true)
{
}

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
    if (m_mergeSourceReferences)
    {
        if (!m_sourceRef)
        {
            m_sourceRef = std::make_shared<SourceReference>(*node->getSourceRef());
        }
        else
        {
            auto &childSourceRef = node->getSourceRef();
            if (childSourceRef->m_col < m_sourceRef->m_col)
            {
                // Child was previous to the one we had saved. Update our col and length.
                m_sourceRef->m_length += m_sourceRef->m_col - childSourceRef->m_col;
                m_sourceRef->m_col = childSourceRef->m_col;
            }
            else
            {
                // Our reference is previous to the child. Only update length.
                m_sourceRef->m_length = childSourceRef->m_col - m_sourceRef->m_col;
                m_sourceRef->m_length += childSourceRef->m_col;
            }
        }
    }

    m_children.push_back(node);
}

void Ast::clearChildren()
{
    m_sourceRef.reset();
    m_children.clear();
}

void Ast::copyChildrenTo(const std::shared_ptr<Ast> &other)
{
    for (auto &child : m_children)
    {
        other->addChild(child);
    }
}

void Ast::setSourceRef(const std::shared_ptr<SourceReference> &ref)
{
    m_sourceRef = ref;
    m_mergeSourceReferences = false;
}

void Ast::setType(AstType type)
{
    m_type = type;
}

const std::shared_ptr<SourceReference> &Ast::getSourceRef()
{
    // If a node doesn't have source reference, it should be extracted from its children.
    if (!m_sourceRef)
    {
        for (auto &child : getChildren())
        {
            if (child->getSourceRef())
                return child->getSourceRef();
        }
    }

    return m_sourceRef;
}

const std::vector<std::shared_ptr<Ast>> &Ast::getChildren()
{
    return m_children;
}
