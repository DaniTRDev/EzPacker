#include "SymbolResolverVisitor/SymbolAnnotation.h"

SymbolAnnotation::SymbolAnnotation(std::shared_ptr<Symbol> symbol) : m_symbol(std::move(symbol)) {}

const char *SymbolAnnotation::getAnnotationName() const { return "SymbolAnnotation"; }

const std::shared_ptr<Symbol> &SymbolAnnotation::getSymbol() const { return m_symbol; }
