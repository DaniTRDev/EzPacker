#include "AstNode/AstNode.h"

bool AstNode::hasAnnotations() const { return m_annotations.size(); }

void AstNode::addAnnotation(const std::shared_ptr<IAstNodeAnnotation> &annotation)
{
    m_annotations.push_front(annotation);
}

void AstNode::setSourceRef(const std::shared_ptr<SourceReference> &ref) { m_sourceRefs.push_back(ref); }

void AstNode::setSourceRef(const std::vector<std::shared_ptr<SourceReference>> &refs) { m_sourceRefs = refs; }

const std::list<std::shared_ptr<IAstNodeAnnotation>> &AstNode::getAnnotations() const { return m_annotations; }

std::shared_ptr<SourceReference> AstNode::getFirstSourceReference() const
{
    if (m_sourceRefs.empty())
        return nullptr;

    return m_sourceRefs[0];
}

const std::vector<std::shared_ptr<SourceReference>> &AstNode::getSourceRefs() const { return m_sourceRefs; }
