#include "Annotations/TypeAnnotation.h"

TypeAnnotation::TypeAnnotation(size_t typeId) { setTypeId(typeId); }

const char *TypeAnnotation::getAnnotationName() { return "Type"; }
