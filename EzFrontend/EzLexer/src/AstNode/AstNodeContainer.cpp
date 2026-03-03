#include "AstNode/AstNodeContainer.h"

bool AstNodeContainer::containsExpressions() const { return getExpressionCount() > 0; }

size_t AstNodeContainer::getExpressionCount() const { return m_expressions->m_numElems; }

TypedPoolSlice<AstNode> *AstNodeContainer::getExpressions() const { return m_expressions; }

void AstNodeContainer::setExpressions(TypedPoolSlice<AstNode> *expressions) { m_expressions = expressions; }
