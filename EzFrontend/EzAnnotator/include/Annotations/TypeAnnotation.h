#ifndef EZPACKER_TYPEANNOTATION_H
#define EZPACKER_TYPEANNOTATION_H

#include "EzAnnotatorCommon.h"
#include "TypeAbleAnnotation.h"

/**
 * This annotation contains a type id. No more information is stored because it could be retrieved from the liked table.
 */
class TypeAnnotation : public TypeAbleAnnotation
{
  public:
    /**
     * Creates the annotation with the given typeId.
     * @param typeId
     */
    TypeAnnotation(size_t typeId);

    /**
     * Returns "Type".
     * @return const char*
     */
    const char *getAnnotationName() override;
};

#endif // EZPACKER_TYPEANNOTATION_H
