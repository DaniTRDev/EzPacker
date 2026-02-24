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
    ScopeAnnotation(const std::shared_ptr<Scope> &scope);

    /**
     * Returns "ScopeAnnotation".
     * @return const char*
     */
    const char *getAnnotationName() const override;
    
    /**
     * Sets the owned scope of this annotation.
     * @param scope
     */
    void setOwnedScope(const std::shared_ptr<Scope> &scope);

    /**
     * Returns the owned scope of this annotation.
     * @return const std::shared_ptr<Scope> &
     */
    const std::shared_ptr<Scope> &getOwnedScope();

  private:
    std::shared_ptr<Scope> m_ownedScope;
};

#endif // EZPACKER_SCOPEANNOTATION_H
