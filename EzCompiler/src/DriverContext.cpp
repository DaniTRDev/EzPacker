#include "DriverContext.h"

namespace EzCompiler
{

DriverContext::DriverContext(const CommandLineOptions &options) :
    m_options(options), m_sessionArena(1024 * 1024) // 1MB initial session arena buffer
{
}

bool DriverContext::initialize()
{
    // current_path() can throw; fall back to a relative base rather than aborting the driver.
    std::error_code cwdEc;
    std::filesystem::path workingDir = std::filesystem::current_path(cwdEc);
    if (cwdEc)
    {
        workingDir = std::filesystem::path(".");
    }

    m_sourceManager = std::make_unique<SourceManager>(workingDir, &m_sessionArena);
    m_diagCollector = std::make_unique<DiagnosticCollector>();
    m_diagLogger = std::make_unique<DiagnosticLogger>(m_sourceManager.get());

    // Apply the requested severity threshold to the collector (default keeps error+warning).
    DiagnosticMessageType enabled = static_cast<DiagnosticMessageType>(
            DiagnosticMessageType::Diag_Error | DiagnosticMessageType::Diag_Warning);
    switch (m_options.diagThreshold)
    {
        case DiagnosticMessageType::Diag_Error:
            enabled = DiagnosticMessageType::Diag_Error;
            break;
        case DiagnosticMessageType::Diag_Trace:
            enabled = static_cast<DiagnosticMessageType>(enabled | DiagnosticMessageType::Diag_Trace);
            break;
        case DiagnosticMessageType::Diag_Debug:
            enabled = static_cast<DiagnosticMessageType>(
                    enabled | DiagnosticMessageType::Diag_Trace | DiagnosticMessageType::Diag_Debug);
            break;
        default:
            break;
    }
    m_diagCollector->setEnabledDiags(enabled);

    // Route collected diagnostics through the logger for formatted output.
    m_diagCollector->addListener(m_diagLogger.get());

    m_typeTable = std::make_unique<MirTypeTable>(&m_sessionArena);
    m_typeTable->initialize(64);

    m_builderCtx =
            std::make_unique<MirBuilderContext>(nullptr, m_diagCollector.get(), m_typeTable.get(), &m_sessionArena);

    // Select concrete target descriptors for the requested triple.
    auto resolved = TargetResolver::resolve(m_options.target, m_builderCtx.get(), m_options.isPositionIndependent);
    if (!resolved.m_targetDesc)
    {
        return false;
    }

    m_resolved = std::move(resolved);

    // Adopt the target's default calling convention for subsequently built functions.
    if (m_resolved.m_callingConv)
    {
        m_builderCtx->setDefaultCallingConvention(m_resolved.m_callingConv);
    }

    return true;
}

std::pmr::memory_resource *DriverContext::getSessionAllocator() { return &m_sessionArena; }

} // namespace EzCompiler
