#include "SemanticAnnotations/ScopeAnnotation.h"

ScopeAnnotation::ScopeAnnotation() : m_ownedScope(nullptr) {}

ScopeAnnotation::ScopeAnnotation(const std::shared_ptr<Scope> &scope) : m_ownedScope(scope) {}

const char *ScopeAnnotation::getAnnotationName() const { return "ScopeAnnotation"; }

void ScopeAnnotation::setOwnedScope(const std::shared_ptr<Scope> &scope) { m_ownedScope = scope; }

const std::shared_ptr<Scope> &ScopeAnnotation::getOwnedScope() { return m_ownedScope; }
