#include "Annotations/SymbolAnnotation.h"

SymbolAnnotation::SymbolAnnotation(size_t symbolId, size_t typeId)
{
    setTypeId(typeId);
    setSymbolId(symbolId);
}

const char *SymbolAnnotation::getAnnotationName() { return "Symbol"; }