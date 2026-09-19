#ifndef EZPACKER_DRIVER_CONTEXT_H
#define EZPACKER_DRIVER_CONTEXT_H

#include "EzCompilerCommon.h"
#include "CommandLineOptions.h"
#include "TargetResolver.h"
#include "SourceManager/SourceManager.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Type/MirTypeTable.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetDesc.h"
#include "Descriptors/TargetBinaryDesc.h"
#include "Function/CallingConvDesc.h"

namespace EzCompiler
{

/**
 * Top-level compiler execution context managing memory resources, diagnostics,
 * source tracking, target architecture descriptors, and intermediate representation states.
 */
class DriverContext
{
  public:
    /**
     * Stores the parsed options; resources are created later by initialize().
     */
    explicit DriverContext(const CommandLineOptions &options);
    ~DriverContext() = default;

    // Non-copyable, non-movable
    DriverContext(const DriverContext &) = delete;
    DriverContext &operator=(const DriverContext &) = delete;

    /**
     * Creates the session allocator, diagnostics, type table, builder context and
     * resolves the target descriptors. Returns false when target resolution fails.
     */
    bool initialize();

    /**
     * Returns the arena that lives for the whole compilation session.
     */
    std::pmr::memory_resource *getSessionAllocator();

    /**
     * Returns the arena used for per-function allocations, creating it on first use.
     */
    std::pmr::memory_resource *getFunctionAllocator();

    /**
     * Discards the per-function arena so the next function starts with fresh memory.
     */
    void resetFunctionAllocator();

    const CommandLineOptions &getOptions() const { return m_options; } ///< Read-only options accessor.
    CommandLineOptions &getOptions() { return m_options; }             ///< Mutable options accessor.

    SourceManager *getSourceManager() { return m_sourceManager.get(); }       ///< Source file tracker.
    DiagnosticCollector *getDiagCollector() { return m_diagCollector.get(); } ///< Diagnostic sink.
    DiagnosticLogger *getDiagLogger() { return m_diagLogger.get(); }          ///< Diagnostic formatter/printer.
    MirTypeTable *getTypeTable() { return m_typeTable.get(); }                ///< Shared MIR type table.
    MirBuilderContext *getBuilderContext() { return m_builderCtx.get(); }     ///< Module/function builder context.

    TargetDesc *getTargetDesc() { return m_targetDesc.get(); }  ///< Resolved target descriptor.
    CallingConvDesc *getCallingConv() { return m_callingConv; } ///< Resolved calling convention.
    TargetBinaryDesc *getBinaryDesc() { return m_binaryDesc; }  ///< Resolved binary/object-format descriptor.

  private:
    CommandLineOptions m_options; ///< Parsed invocation options.

    std::pmr::monotonic_buffer_resource m_sessionArena;                   ///< Session-lifetime arena (1MB initial).
    std::unique_ptr<std::pmr::monotonic_buffer_resource> m_functionArena; ///< Per-function arena, reset per function.

    std::unique_ptr<SourceManager> m_sourceManager;       ///< Owns source file buffers and locations.
    std::unique_ptr<DiagnosticCollector> m_diagCollector; ///< Owns collected diagnostics.
    std::unique_ptr<DiagnosticLogger> m_diagLogger;       ///< Owns the diagnostic listener/logger.
    std::unique_ptr<MirTypeTable> m_typeTable;            ///< Owns the MIR type table.
    std::unique_ptr<MirBuilderContext> m_builderCtx;      ///< Owns the MIR module builder context.

    std::unique_ptr<TargetDesc> m_targetDesc;  ///< Owns the selected target descriptor.
    CallingConvDesc *m_callingConv{ nullptr }; ///< Non-owning pointer into the target's calling convention.
    TargetBinaryDesc *m_binaryDesc{ nullptr }; ///< Non-owning pointer into the target's binary descriptor.
};

} // namespace EzCompiler

#endif // EZPACKER_DRIVER_CONTEXT_H
