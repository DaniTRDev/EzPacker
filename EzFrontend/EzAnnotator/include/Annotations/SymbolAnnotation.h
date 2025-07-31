#ifndef EZPACKER_SYMBOLANNOTATION_H
#define EZPACKER_SYMBOLANNOTATION_H

#include "EzAnnotatorCommon.h"
#include "TypeAbleAnnotation.h"
#include "SymbolAbleAnnotation.h"

/**
 * This class represents a whole symbol, with its typeId (TypeAbleAnnotation) and symbolId(SymbolAbleAnnotation).
 */
class SymbolAnnotation : public TypeAbleAnnotation, public SymbolAbleAnnotation
{
  public:
    /**
     * Builds the annotation with the given symbolId and typeId.
     * @param symbolId
     * @param typeId
     */
    SymbolAnnotation(size_t symbolId, size_t typeId);

    /**
     * Returns the name "Symbol".
     * @return const char *
     */
    const char *getAnnotationName() override;
};

#endif // EZPACKER_SYMBOLANNOTATION_H
