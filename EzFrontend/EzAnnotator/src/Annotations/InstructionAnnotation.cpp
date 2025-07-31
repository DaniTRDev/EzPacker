#include "Annotations/InstructionAnnotation.h"

InstructionAnnotation::InstructionAnnotation(size_t id) : m_id(id) {}

const char *InstructionAnnotation::getAnnotationName() { return "Instruction"; }

size_t InstructionAnnotation::getInstrId() const { return m_id; }
