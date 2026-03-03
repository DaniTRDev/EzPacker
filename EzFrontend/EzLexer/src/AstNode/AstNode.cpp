#include "AstNode/AstNode.h"

bool AstNode::hasAnnotations() const { return m_annotations && m_annotations->m_numElems != 0; }

const SourceReference &AstNode::getSourceRef() const { return m_sourceRef; }

TypedPoolSlice<IAstNodeAnnotation> *AstNode::getAnnotations() { return m_annotations; }

void AstNode::setSourceRefs(const SourceReference &refs) { m_sourceRef = refs; }

void AstNode::addAnnotation(IAstNodeAnnotation *annot, TypedPool *annotPool)
{
    if (!annotPool || !annot)
    {
        throw std::runtime_error("Internal compiler error: Tries to annotate something invalid or pool is not valid");
    }

    if (!m_annotations)
    {
        m_annotations = annotPool->createSlice<IAstNodeAnnotation>();
    }

    annotPool->appendToSliceInFront<IAstNodeAnnotation>(m_annotations, annot);
}
