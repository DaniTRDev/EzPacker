#include "SemanticAnnotations/DataTypeAnnotation.h"

DataTypeAnnotation::DataTypeAnnotation(Type *type) : m_type(type) {}

const char *DataTypeAnnotation::getAnnotationName() const { return "DataTypeAnnotation"; }

Type *DataTypeAnnotation::getDataType() const { return m_type; }
