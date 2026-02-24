#include "SemanticAnnotations/ScopedSymbolAnnotation.h"

const char *ScopedSymbolAnnotation::getAnnotationName() const { return "ScopedSymbolAnnotation"; }

void ScopedSymbolAnnotation::setOwnedScope(const std::shared_ptr<Scope> &scope) { m_ownedScope = scope; }

const std::shared_ptr<Scope> &ScopedSymbolAnnotation::getOwnedScope() { return m_ownedScope; }
