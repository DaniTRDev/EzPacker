#include "SemanticAnnotations/ScopeAnnotation.h"

ScopeAnnotation::ScopeAnnotation() : m_ownedScope(nullptr) {}

ScopeAnnotation::ScopeAnnotation(Scope *scope) : m_ownedScope(scope) {}

const char *ScopeAnnotation::getAnnotationName() const { return "ScopeAnnotation"; }

void ScopeAnnotation::setOwnedScope(Scope *scope) { m_ownedScope = scope; }

Scope *ScopeAnnotation::getOwnedScope() { return m_ownedScope; }
