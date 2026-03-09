/**
 * @file SymbolAnnotation.h
 * @brief Annotation that links an AST node to a resolved semantic symbol.
 *
 * This is the primary bridge between syntax and the symbol table. It can be
 * attached either at a declaration site (for example, when a variable/module/
 * label definition creates its symbol) or at a use site after the resolver
 * matches a name reference to a declaration.
 */
#ifndef EZPACKER_SYMBOLANNOTATION_H
#define EZPACKER_SYMBOLANNOTATION_H

#include "EzSemanticsCommon.h"
#include "Scope/Symbol.h"

/**
 * Annotation that stores the `Symbol` associated with an AST node.
 */
class SymbolAnnotation : public IAstNodeAnnotation
{
  public:
    /**
     * Creates an annotation with no associated symbol yet.
     */
    explicit SymbolAnnotation();

    /**
     * Creates the annotation pointing at an already resolved symbol.
     */
    SymbolAnnotation(Symbol *symbol);

    /**
     * Returns the associated semantic symbol.
     */
    Symbol *getSymbol() const;

    /**
     * Returns the runtime annotation kind name: `"SymbolAnnotation"`.
     */
    const char *getAnnotationName() const override;

    /**
     * Stores the associated semantic symbol.
     */
    void setSymbol(Symbol *symbol);

  private:
    Symbol *m_symbol;
};

#endif // EZPACKER_SYMBOLANNOTATION_H
