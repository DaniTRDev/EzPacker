#include "DriverContext.h"

namespace EzCompiler
{

DriverContext::DriverContext(const CommandLineOptions &options) :
    m_options(options),
    m_sessionArena(1024 * 1024) // 1MB initial session arena buffer
{
}

bool DriverContext::initialize()
{
    m_sourceManager = std::make_unique<SourceManager>(std::filesystem::current_path(), &m_sessionArena);
    m_diagCollector = std::make_unique<DiagnosticCollector>();
    m_diagLogger = std::make_unique<DiagnosticLogger>(m_sourceManager.get());

    m_diagCollector->addListener(m_diagLogger.get());

    m_typeTable = std::make_unique<MirTypeTable>(&m_sessionArena);
    m_typeTable->initialize(64);

    m_builderCtx = std::make_unique<MirBuilderContext>(nullptr, m_diagCollector.get(), m_typeTable.get(), &m_sessionArena);

    auto resolved = TargetResolver::resolve(m_options.target, m_builderCtx.get());
    if (!resolved.m_targetDesc)
    {
        return false;
    }

    m_targetDesc = std::move(resolved.m_targetDesc);
    m_callingConv = resolved.m_callingConv;
    m_binaryDesc = resolved.m_binaryDesc;

    if (m_callingConv)
    {
        m_builderCtx->setDefaultCallingConvention(m_callingConv);
    }

    m_functionArena = std::make_unique<std::pmr::monotonic_buffer_resource>(64 * 1024); // 64KB initial func arena

    return true;
}

std::pmr::memory_resource *DriverContext::getSessionAllocator()
{
    return &m_sessionArena;
}

std::pmr::memory_resource *DriverContext::getFunctionAllocator()
{
    if (!m_functionArena)
    {
        m_functionArena = std::make_unique<std::pmr::monotonic_buffer_resource>(64 * 1024);
    }
    return m_functionArena.get();
}

void DriverContext::resetFunctionAllocator()
{
    m_functionArena = std::make_unique<std::pmr::monotonic_buffer_resource>(64 * 1024);
}

} // namespace EzCompiler
