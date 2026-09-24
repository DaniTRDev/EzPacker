#ifndef EZPACKER_EMISSION_ENGINE_H
#define EZPACKER_EMISSION_ENGINE_H

#include "EzCompilerCommon.h"

class MirBuilderContext;

namespace EzCompiler
{

class DriverContext;

/**
 * Machine code emission engine bridging lowered MIR representations into hardware bytecode
 * and packaging final object files (ELF64 / PE-COFF).
 */
class EmissionEngine
{
  public:
    /**
     * Binds the engine to the driver context that supplies diagnostics, symbols and the allocator.
     */
    explicit EmissionEngine(DriverContext &ctx);

    /**
     * Serializes all lowered functions in mirCtx and writes the final binary object file to outputPath.
     */
    bool emitModule(MirBuilderContext &mirCtx, std::string_view outputPath);

  private:
    DriverContext &m_ctx; ///< Driver context supplying diagnostics, target descriptors and allocators.
};

} // namespace EzCompiler

#endif // EZPACKER_EMISSION_ENGINE_H
