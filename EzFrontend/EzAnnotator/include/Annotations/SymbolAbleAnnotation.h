#ifndef EZPACKER_SYMBOLABLEANNOTATION_H
#define EZPACKER_SYMBOLABLEANNOTATION_H

#include "EzAnnotatorCommon.h"

/**
 * This class is a simple utility class that can be used in inheritance to give an annotation the ability of having
 * a symbol. Creating this class has been a strategic design decision because many annotations require having a
 * symbol, and this allows us to quickly change, in every single one of them, how they saved their symbol information.
 *
 * This class only saves the symbol id, it doesn't save the type nor anything else.
 */
class SymbolAbleAnnotation : public virtual IAstNodeAnnotation
{
  public:
    /**
     * Returns the ID of the symbol.
     * @return size_t
     */
    size_t getSymbolId() const;

    /**
     * Sets the ID of the symbol.
     * @param symbolId
     */
    void setSymbolId(size_t symbolId);

  private:
    size_t m_symbolId;
};

#endif // EZPACKER_SYMBOLABLEANNOTATION_H
