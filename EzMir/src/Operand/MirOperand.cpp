#include "Operand/MirOperand.h"
#include "SourceManager/SourceManager.h"
#include "Type/MirType.h"

/**
 * Initializes the base operand with its associated MIR type and source reference.
 */
MirOperand::MirOperand(MirType *type, SourceReference *sourceRef) : m_type(type), m_sourceRef(sourceRef) {}

/**
 * Returns the MIR type descriptor associated with this operand.
 */
MirType *MirOperand::getMirType() const { return m_type; }

/**
 * Computes the size in bytes of the operand based on its MIR type.
 */
size_t MirOperand::getSizeInBytes() const { return m_type ? m_type->getTotalSizeInBytes() : 0; }

/**
 * Returns the source location reference for diagnostics.
 */
SourceReference *MirOperand::getSourceRef() const { return m_sourceRef; }

/**
 * Updates the MIR type descriptor for this operand.
 */
void MirOperand::setMirType(MirType *type) { m_type = type; }
