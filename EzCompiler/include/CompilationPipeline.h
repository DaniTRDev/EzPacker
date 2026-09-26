#ifndef EZPACKER_COMPILATION_PIPELINE_H
#define EZPACKER_COMPILATION_PIPELINE_H

#include "EzCompilerCommon.h"
#include "CommandLineOptions.h"

class MirBuilderContext;
class MirFunction;
class MirPassManager;

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
    /**
     * Binds the pipeline to the driver context that owns diagnostics, allocators and targets.
     */
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
    /**
     * Runs input MIR verification (flags, operand kinds and sizes, structural invariants) on a function.
     * Returns false if verification fails.
     */
    bool runInputVerification(MirFunction *func, MirPassManager &passManager);

    /**
     * Runs CFG analysis, SSA construction and liveness analysis on a function.
     * Returns false if a pass reports failure.
     */
    bool runMiddleEndPasses(MirFunction *func, MirPassManager &passManager);

    /**
     * Runs function-signature and operation legalization passes on a function.
     * Returns false if a pass reports failure.
     */
    bool runLegalizationPasses(MirFunction *func, MirPassManager &passManager);

    /**
     * Runs ABI lowering, instruction selection, register allocation and frame lowering.
     * Returns false if a pass reports failure.
     */
    bool runTargetLoweringPasses(MirFunction *func, MirPassManager &passManager);

    /**
     * Prints the "Running <pass>" banner for a function when --print-passes is enabled.
     */
    void printPassRunning(std::string_view passName, const MirFunction *func) const;

    /**
     * Reports a failed pass through the diagnostic collector and returns false.
     */
    bool reportPassFailure(const char *message);

    /**
     * Constructs, times and runs one pass on a single function, centralizing the print/timing/error
     * scaffolding shared by every stage. Returns false when the pass reports failure.
     */
    template <typename PassT, typename... Args>
    bool runCheckedPass(std::string_view passName,
                        MirFunction *func,
                        MirPassManager *passManager,
                        const char *failureMessage,
                        Args &&...args);

  private:
    DriverContext &m_ctx; ///< Driver context supplying diagnostics, allocators and target descriptors.
};

} // namespace EzCompiler

#endif // EZPACKER_COMPILATION_PIPELINE_H
