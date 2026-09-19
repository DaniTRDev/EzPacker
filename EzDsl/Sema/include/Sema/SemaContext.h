#ifndef EZDSL_SEMA_CONTEXT_H
#define EZDSL_SEMA_CONTEXT_H

#include "EzDslSemaCommon.h"

/**
 * Unified context passed across semantic analysis passes in EzDsl.
 * Holds references to the global DiagnosticCollector, SourceManager, and SymbolTable.
 */
class SemaContext
{
  public:
    /**
     * Constructs a semantic analysis context with the given diagnostic collector, source manager, and symbol table.
     */
    SemaContext(class DiagnosticCollector *diagCollector,
                class SourceManager *sourceManager,
                class SymbolTable *symbolTable);

    /**
     * Returns the diagnostic collector linked to this semantic analysis session.
     */
    class DiagnosticCollector *getDiagCollector() const;

    /**
     * Returns the concrete source manager managing loaded source files.
     */
    class SourceManager *getSourceManager() const;

    /**
     * Returns the symbol table holding declared types, opcodes, registers, and rules.
     */
    class SymbolTable *getSymTable() const;

  private:
    class DiagnosticCollector *m_diagCollector; // Collector receiving semantic diagnostics.
    class SourceManager *m_sourceManager;       // Source manager used to resolve diagnostic spans.
    class SymbolTable *m_symbolTable;           // Symbol table shared across passes.
};

#endif // EZDSL_SEMA_CONTEXT_H