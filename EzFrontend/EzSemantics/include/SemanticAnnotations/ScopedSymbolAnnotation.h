#ifndef EZPACKER_SCOPEDSYMBOLANNOTATION_H
#define EZPACKER_SCOPEDSYMBOLANNOTATION_H

#include "EzSemanticsCommon.h"
#include "ScopeAnnotation.h"
#include "SymbolAnnotation.h"

/**
 * Annotation used for nodes that define a symbol and OWN a scope. This class inherits ONLY from the symbol annotation
 * to avoid "Diamond Problem" (2 classes inheriting from the same class).
 */
class ScopedSymbolAnnotation : public SymbolAnnotation
{
  public:
    /**
     * Returns "ScopedSymbolAnnotation".
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

#endif // EZPACKER_SCOPEDSYMBOLANNOTATION_H
