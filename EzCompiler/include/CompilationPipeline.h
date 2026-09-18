#ifndef EZPACKER_COMPILATION_PIPELINE_H
#define EZPACKER_COMPILATION_PIPELINE_H

#include "EzCompilerCommon.h"
#include "CommandLineOptions.h"

class MirBuilderContext;
class MirFunction;

namespace EzCompiler
{

class DriverContext;

/**
 * Orchestrates middle-end SSA/CFG optimization passes and EzTriple backend lowering transformations.
 * Respects inspection gates (--emit-mir, --emit-legalized-mir, --emit-lowered-mir, --emit-asm).
 */
class CompilationPipeline
{
  public:
    explicit CompilationPipeline(DriverContext &ctx);

    /**
     * Executes the full pass sequence across all functions in the module.
     * Returns true if pipeline completed successfully (or cleanly early-exited at an inspection gate).
     */
    bool runPipeline();

    /**
     * Dumps human-readable representation of functions according to the current emission stage.
     */
    std::string dumpCurrentMir() const;

    /**
     * Dumps assembly-like instruction listings for lowered functions.
     */
    std::string dumpAssembly() const;

  private:
    bool runMiddleEndPasses(MirFunction *func);
    bool runLegalizationPasses(MirFunction *func);
    bool runTargetLoweringPasses(MirFunction *func);

  private:
    DriverContext &m_ctx;
};

} // namespace EzCompiler

#endif // EZPACKER_COMPILATION_PIPELINE_H
