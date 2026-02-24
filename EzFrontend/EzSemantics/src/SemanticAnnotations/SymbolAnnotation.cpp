#include "SemanticAnnotations/SymbolAnnotation.h"

SymbolAnnotation::SymbolAnnotation() : m_symbol(nullptr) {}

SymbolAnnotation::SymbolAnnotation(const std::shared_ptr<Symbol> &symbol) : m_symbol(symbol) {}

const char *SymbolAnnotation::getAnnotationName() const { return "SymbolAnnotation"; }

void SymbolAnnotation::setSymbol(const std::shared_ptr<Symbol> &symbol) { m_symbol = symbol; }

const std::shared_ptr<Symbol> &SymbolAnnotation::getSymbol() const { return m_symbol; }
