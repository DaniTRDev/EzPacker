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
    explicit DriverContext(const CommandLineOptions &options);
    ~DriverContext() = default;

    // Non-copyable, non-movable
    DriverContext(const DriverContext &) = delete;
    DriverContext &operator=(const DriverContext &) = delete;

    bool initialize();

    std::pmr::memory_resource *getSessionAllocator();
    std::pmr::memory_resource *getFunctionAllocator();
    void resetFunctionAllocator();

    const CommandLineOptions &getOptions() const { return m_options; }
    CommandLineOptions &getOptions() { return m_options; }

    SourceManager *getSourceManager() { return m_sourceManager.get(); }
    DiagnosticCollector *getDiagCollector() { return m_diagCollector.get(); }
    DiagnosticLogger *getDiagLogger() { return m_diagLogger.get(); }
    MirTypeTable *getTypeTable() { return m_typeTable.get(); }
    MirBuilderContext *getBuilderContext() { return m_builderCtx.get(); }

    TargetDesc *getTargetDesc() { return m_targetDesc.get(); }
    CallingConvDesc *getCallingConv() { return m_callingConv; }
    TargetBinaryDesc *getBinaryDesc() { return m_binaryDesc; }

  private:
    CommandLineOptions m_options;

    std::pmr::monotonic_buffer_resource m_sessionArena;
    std::unique_ptr<std::pmr::monotonic_buffer_resource> m_functionArena;

    std::unique_ptr<SourceManager> m_sourceManager;
    std::unique_ptr<DiagnosticCollector> m_diagCollector;
    std::unique_ptr<DiagnosticLogger> m_diagLogger;
    std::unique_ptr<MirTypeTable> m_typeTable;
    std::unique_ptr<MirBuilderContext> m_builderCtx;

    std::unique_ptr<TargetDesc> m_targetDesc;
    CallingConvDesc *m_callingConv{ nullptr };
    TargetBinaryDesc *m_binaryDesc{ nullptr };
};

} // namespace EzCompiler

#endif // EZPACKER_DRIVER_CONTEXT_H
