#ifndef EZPACKER_SYMBOLANNOTATION_H
#define EZPACKER_SYMBOLANNOTATION_H

#include "EzSemanticsCommon.h"
#include "Symbol.h"

class SymbolAnnotation : public IAstNodeAnnotation
{
  public:
    /**
     * Creates the annotation with the given symbol.
     * @param symbol
     */
    SymbolAnnotation(std::shared_ptr<Symbol> symbol);

    /**
     * Returns "SymbolAnnotation".
     * @return const char*
     */
    const char *getAnnotationName() const override;
    
    /**
     * Returns the symbol linked to this annotation.
     * @return const std::shared_ptr<Symbol> &
     */
    const std::shared_ptr<Symbol> &getSymbol() const;
    
  private:
    std::shared_ptr<Symbol> m_symbol;
};

#endif // EZPACKER_SYMBOLANNOTATION_H
