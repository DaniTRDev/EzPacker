/**
 * @file SymbolAnnotation.h
 * @brief Annotation that links an AST node to its defining Symbol.
 *
 * Attached to Variable, Label, and Module nodes after
 * SymbolDefinitionVisitor creates their symbols.  Later passes and the
 * lowerer read this annotation to look up type information, MIR IDs, and
 * other symbol metadata.
 */
#ifndef EZPACKER_SYMBOLANNOTATION_H
#define EZPACKER_SYMBOLANNOTATION_H

#include "EzSemanticsCommon.h"
#include "Scope/Symbol.h"

/**
 * Annotation used for nodes that are symbols.
 */
class SymbolAnnotation : public IAstNodeAnnotation
{
  public:
    /**
     * Creates a default object WITHOUT a symbol.
     */
    explicit SymbolAnnotation();

    /**
     * Creates the annotation linked to the given symbol.
     * @param symbol
     */
    SymbolAnnotation(Symbol *symbol);

    /**
     * Returns the symbol of this annotation.
     * @return Symbol *
     */
    Symbol *getSymbol() const;

    /**
     * Returns "SymbolAnnotation".
     * @return const char*
     */
    const char *getAnnotationName() const override;

    /**
     * Sets the symbol of this annotation.
     * @param symbol
     */
    void setSymbol(Symbol *symbol);

  private:
    Symbol *m_symbol;
};

#endif // EZPACKER_SYMBOLANNOTATION_H
