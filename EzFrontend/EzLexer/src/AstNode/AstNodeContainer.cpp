#include "AstNode/AstNodeContainer.h"

bool AstNodeContainer::containsExpressions() const { return getExpressionCount() > 0; }

size_t AstNodeContainer::getExpressionCount() const { return m_expressions->m_numElems; }

TypedPoolLinkedList<AstNode> *AstNodeContainer::getExpressions() const { return m_expressions; }

void AstNodeContainer::setExpressions(TypedPoolLinkedList<AstNode> *expressions) { m_expressions = expressions; }
