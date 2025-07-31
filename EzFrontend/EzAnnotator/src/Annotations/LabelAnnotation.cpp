#include "Annotations/LabelAnnotation.h"

LabelAnnotation::LabelAnnotation(size_t symbolId) { setSymbolId(symbolId); }

const char *LabelAnnotation::getAnnotationName() { return "Label"; }
