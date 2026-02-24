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
    SymbolAnnotation();

    /**
     * Creates the annotation linked to the given symbol.
     * @param symbol
     */
    SymbolAnnotation(const std::shared_ptr<Symbol> &symbol);

    /**
     * Returns "SymbolAnnotation".
     * @return const char*
     */
    const char *getAnnotationName() const override;

    /**
     * Sets the symbol of this annotation.
     * @param symbol
     */
    void setSymbol(const std::shared_ptr<Symbol> &symbol);

    /**
     * Returns the symbol of this annotation.
     * @return const std::shared_ptr<Symbol> &
     */
    const std::shared_ptr<Symbol> &getSymbol() const;

  private:
    std::shared_ptr<Symbol> m_symbol;
};

#endif // EZPACKER_SYMBOLANNOTATION_H
