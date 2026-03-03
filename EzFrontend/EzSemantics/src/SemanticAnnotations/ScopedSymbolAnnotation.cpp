#include "SemanticAnnotations/ScopedSymbolAnnotation.h"

const char *ScopedSymbolAnnotation::getAnnotationName() const { return "ScopedSymbolAnnotation"; }

Scope *ScopedSymbolAnnotation::getOwnedScope() { return m_ownedScope; }

void ScopedSymbolAnnotation::setOwnedScope(Scope *scope) { m_ownedScope = scope; }
