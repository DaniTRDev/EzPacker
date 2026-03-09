/**
 * @file ScopedSymbolAnnotation.h
 * @brief Annotation for AST nodes that both define a symbol and own a scope.
 *
 * This is the combined annotation used by declarations such as `Module` and
 * `Label`, where one AST node introduces a named symbol into its parent scope
 * and simultaneously owns a nested body scope. It extends `SymbolAnnotation`
 * and adds a non-owning pointer to the owned `Scope`.
 */
#ifndef EZPACKER_SCOPEDSYMBOLANNOTATION_H
#define EZPACKER_SCOPEDSYMBOLANNOTATION_H

#include "EzSemanticsCommon.h"
#include "ScopeAnnotation.h"
#include "SymbolAnnotation.h"

/**
 * Combined annotation for nodes that define a symbol and own a child scope.
 */
class ScopedSymbolAnnotation : public SymbolAnnotation
{
  public:
    /**
     * Returns the runtime annotation kind name: `"ScopedSymbolAnnotation"`.
     */
    const char *getAnnotationName() const override;

    /**
     * Returns the scope owned by the annotated declaration node.
     */
    Scope *getOwnedScope();

    /**
     * Stores the scope owned by the annotated declaration node.
     */
    void setOwnedScope(Scope *scope);

  private:
    Scope *m_ownedScope;
};

#endif // EZPACKER_SCOPEDSYMBOLANNOTATION_H
