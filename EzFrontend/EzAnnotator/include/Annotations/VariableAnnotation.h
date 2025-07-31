#ifndef EZPACKER_VARIABLEANNOTATION_H
#define EZPACKER_VARIABLEANNOTATION_H

#include "EzAnnotatorCommon.h"
#include "SymbolAbleAnnotation.h"
#include "TypeAbleAnnotation.h"

/**
 * This annotation works more or less like a module annotation:
 *  - Has the type of the variable (TypeAbleAnnotation).
 *  - Has the symbol of the veriable so it can be referenced (SymbolAbleAnnotation).
 *  - Initializers are saved as children of the VariableNode holding this annotation. They will be annotated on their
 *    own with a ConstantAnnotation.
 *
 *    Even if this class is empty, it's very very susceptible of being expanded in a future when the language grows.
 */
class VariableAnnotation : public SymbolAbleAnnotation, public TypeAbleAnnotation
{
  public:
    /**
     * Creates the annotation with the given symbol and type IDs.
     * @param symbolId
     * @param typeId
     */
    VariableAnnotation(size_t symbolId, size_t typeId);
};

#endif // EZPACKER_VARIABLEANNOTATION_H
