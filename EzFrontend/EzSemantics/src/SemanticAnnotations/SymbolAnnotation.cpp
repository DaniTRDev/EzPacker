#include "SemanticAnnotations/SymbolAnnotation.h"

SymbolAnnotation::SymbolAnnotation() : m_symbol(nullptr) {}

SymbolAnnotation::SymbolAnnotation(Symbol *symbol) : m_symbol(symbol) {}

Symbol *SymbolAnnotation::getSymbol() const { return m_symbol; }

const char *SymbolAnnotation::getAnnotationName() const { return "SymbolAnnotation"; }

void SymbolAnnotation::setSymbol(Symbol *symbol) { m_symbol = symbol; }
