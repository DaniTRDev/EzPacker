#include "Annotations/MemoryRefAnnotation.h"

MemoryRefAnnotation::MemoryRefAnnotation(MemoryReferenceType refType,
                                         size_t baseId,
                                         size_t indexId,
                                         size_t typeId,
                                         const std::shared_ptr<ConstantAnnotation> &displ,
                                         const std::shared_ptr<ConstantAnnotation> &scale) :
    m_type(refType), m_baseId(baseId), m_indexId(indexId), m_displ(displ), m_scale(scale)
{
    setTypeId(typeId);
}

const char *MemoryRefAnnotation::getAnnotationName() { return "MemoryReference"; }

MemoryReferenceType MemoryRefAnnotation::getRefType() const { return m_type; }

size_t MemoryRefAnnotation::getBaseId() const { return m_baseId; }

size_t MemoryRefAnnotation::getIndexId() const { return m_indexId; }

const std::shared_ptr<ConstantAnnotation> &MemoryRefAnnotation::getDispl() const { return m_displ; }

const std::shared_ptr<ConstantAnnotation> &MemoryRefAnnotation::getScale() const { return m_scale; }
