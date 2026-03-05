/**
 * @file ScopeAnnotation.h
 * @brief Annotation for AST nodes that own a lexical scope.
 *
 * Attached to nodes like CodeScope during SymbolDefinitionVisitor.  It
 * holds a shared_ptr<Scope> so that later passes (resolution, type
 * checking, lowering) can re-enter the correct scope when they visit the
 * annotated node.
 */
#ifndef EZPACKER_SCOPEANNOTATION_H
#define EZPACKER_SCOPEANNOTATION_H

#include "EzSemanticsCommon.h"
#include "Scope/Scope.h"

/**
 * Annotation used for nodes that OWN a scope.
 */
class ScopeAnnotation : public IAstNodeAnnotation
{
  public:
    /**
     * Creates a default object WITHOUT a scope.
     */
    ScopeAnnotation();

    /**
     * Creates the annotation linked to the given scope.
     * @param scope
     */
    ScopeAnnotation(Scope *scope);

    /**
     * Returns "ScopeAnnotation".
     * @return const char*
     */
    const char *getAnnotationName() const override;
    
    /**
     * Sets the owned scope of this annotation.
     * @param scope
     */
    void setOwnedScope(Scope *scope);

    /**
     * Returns the owned scope of this annotation.
     * @return Scope *
     */
    Scope *getOwnedScope();

  private:
    Scope * m_ownedScope;
};

#endif // EZPACKER_SCOPEANNOTATION_H
