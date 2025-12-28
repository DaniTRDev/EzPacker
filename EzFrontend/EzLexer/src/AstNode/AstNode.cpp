#include "AstNode/AstNode.h"

void AstNode::setAnnotation(const std::shared_ptr<IAstNodeAnnotation> &annotation) { m_annotation = annotation; }

void AstNode::setSourceRef(const std::shared_ptr<SourceReference> &ref) { m_sourceRef = ref; }

const std::shared_ptr<IAstNodeAnnotation> &AstNode::getAnnotation() const { return m_annotation; }

const std::shared_ptr<SourceReference> &AstNode::getSourceRef() const
{
    return m_sourceRef;
}