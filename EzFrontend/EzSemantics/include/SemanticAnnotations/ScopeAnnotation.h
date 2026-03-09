/**
 * @file ScopeAnnotation.h
 * @brief Annotation for AST nodes that own a lexical `Scope`.
 *
 * `SymbolDefinitionVisitor` attaches this annotation to nodes that create a
 * child scope but do not themselves define a named symbol that also needs to
 * be tracked separately. Later passes re-enter that stored scope through
 * `BasicSemanticContext::enterScope()`.
 *
 * The stored pointer is non-owning: the actual `Scope` lifetime is managed by
 * `BasicSemanticContext`.
 */
#ifndef EZPACKER_SCOPEANNOTATION_H
#define EZPACKER_SCOPEANNOTATION_H

#include "EzSemanticsCommon.h"
#include "Scope/Scope.h"

/**
 * Annotation that points to the scope owned by an AST node.
 */
class ScopeAnnotation : public IAstNodeAnnotation
{
  public:
    /**
     * Creates an annotation with no owned scope attached yet.
     */
    ScopeAnnotation();

    /**
     * Creates the annotation already pointing at an existing scope.
     */
    ScopeAnnotation(Scope *scope);

    /**
     * Returns the runtime annotation kind name: `"ScopeAnnotation"`.
     */
    const char *getAnnotationName() const override;

    /**
     * Stores the scope owned by the annotated AST node.
     */
    void setOwnedScope(Scope *scope);

    /**
     * Returns the scope associated with the annotated AST node.
     */
    Scope *getOwnedScope();

  private:
    Scope * m_ownedScope;
};

#endif // EZPACKER_SCOPEANNOTATION_H
