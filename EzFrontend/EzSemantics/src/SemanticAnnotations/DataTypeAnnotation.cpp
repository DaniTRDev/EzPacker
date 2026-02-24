#include "SemanticAnnotations/DataTypeAnnotation.h"

DataTypeAnnotation::DataTypeAnnotation(const std::shared_ptr<Type> &type) : m_type(type) {}

const char *DataTypeAnnotation::getAnnotationName() const { return "DataTypeAnnotation"; }

const std::shared_ptr<Type> &DataTypeAnnotation::getDataType() const { return m_type; }
