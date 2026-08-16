#include "Operand/MirOperand.h"
#include "SourceManager/SourceManager.h"
#include "Type/MirType.h"

MirOperand::MirOperand(MirType *type, SourceReference *sourceRef) : m_type(type), m_sourceRef(sourceRef) {}

MirType *MirOperand::getMirType() const { return m_type; }

size_t MirOperand::getSizeInBytes() const { return m_type ? m_type->getTotalSizeInBytes() : 0; }

SourceReference *MirOperand::getSourceRef() const { return m_sourceRef; }

void MirOperand::setMirType(MirType *type) { m_type = type; }
