#include "Sema/SemaContext.h"

SemaContext::SemaContext(DiagnosticCollector *diagCollector, SourceManager *sourceManager, SymbolTable *symbolTable) :
    m_diagCollector(diagCollector), m_sourceManager(sourceManager), m_symbolTable(symbolTable)
{
}

DiagnosticCollector *SemaContext::getDiagCollector() const { return m_diagCollector; }

SourceManager *SemaContext::getSourceManager() const { return m_sourceManager; }

SymbolTable *SemaContext::getSymTable() const { return m_symbolTable; }