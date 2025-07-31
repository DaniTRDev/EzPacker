#ifndef EZPACKER_LABELANNOTATION_H
#define EZPACKER_LABELANNOTATION_H

#include "EzAnnotatorCommon.h"
#include "SymbolAbleAnnotation.h"

/**
 * This annotation contains information of a label. Internally a label would just be a symbol, but I thought it'd be
 * better to have a whole annotation for it because it will surely be extended in a future.
 */
class LabelAnnotation : public SymbolAbleAnnotation
{
  public:
    /**
     * Creates the label annotation with the given symbol ID.
     * @param symbolId
     */
    LabelAnnotation(size_t symbolId);

    /**
     * Returns "Label"
     * @return const char*
     */
    const char *getAnnotationName() override;
};

#endif // EZPACKER_LABELANNOTATION_H
