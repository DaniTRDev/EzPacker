#ifndef EZPACKER_EMISSION_ENGINE_H
#define EZPACKER_EMISSION_ENGINE_H

#include "EzCompilerCommon.h"

class MirBuilderContext;
class MirFunction;
class CodeEmitterContext;
class GenericCodeEmitter;

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
    explicit EmissionEngine(DriverContext &ctx);

    /**
     * Serializes all lowered functions in mirCtx and writes the final binary object file to outputPath.
     */
    bool emitModule(MirBuilderContext &mirCtx, std::string_view outputPath);

  private:
    bool emitFunction(MirFunction *func, GenericCodeEmitter &emitter, ::CodeEmitterContext &emitterCtx);

  private:
    DriverContext &m_ctx;
};

} // namespace EzCompiler

#endif // EZPACKER_EMISSION_ENGINE_H
