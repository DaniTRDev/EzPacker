#ifndef EZDSL_SEMA_CONTEXT_H
#define EZDSL_SEMA_CONTEXT_H

#include "EzDslCommon.h"

using SemaId = size_t;
constexpr size_t SEMAID_INVALID = 0;

class SemaContext
{
  public:
    /**
     * Creates the context with the given diagnostic collector, source manager and symbol table.
     */
    SemaContext(class DiagnosticCollector *diagCollector,
                class SourceManager *sourceManager,
                class SymbolTable *symbolTable);

    /**
     * Returns the diagnostic collector.
     */
    class DiagnosticCollector *getDiagCollector() const;

    /**
     * Returns the source manager.
     */
    class SourceManager *getSourceManager() const;

    /**
     * Returns the symbol table.
     */
    class SymbolTable *getSymTable() const;

  private:
    class DiagnosticCollector *m_diagCollector;
    class SourceManager *m_sourceManager;
    class SymbolTable *m_symbolTable;
};

#endif // EZDSL_SEMA_CONTEXT_H