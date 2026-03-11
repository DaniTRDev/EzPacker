/**
 * @file FrontendCompilationUnitPhase.h
 * @brief Base interface for all compilation phases.
 *
 * Defines the contract for a single step in the compilation pipeline. Each phase
 * (e.g., Tokenization, Parsing, Semantic Analysis) implements this interface
 * to perform a specific transformation or analysis on a `FrontendCompilationUnit`.
 */
#ifndef EZPACKER_FRONTENDCOMPILATIONUNITPHASE_H
#define EZPACKER_FRONTENDCOMPILATIONUNITPHASE_H

#include "EzFrontendCompilerCommon.h"
#include "FrontendCompilationUnit.h"

/**
 * @class FrontendCompilationUnitPhase
 * @brief Abstract base class for compilation phases.
 *
 * A compilation phase represents a distinct step in processing a source file.
 * Concrete implementations define the logic for tokenization, parsing, etc.
 * The `FrontendCompilerDriver` executes these phases sequentially.
 */
class FrontendCompilationUnitPhase
{
  public:
    virtual ~FrontendCompilationUnitPhase() = default;

    /**
     * @brief Executes the phase on the given compilation unit.
     *
     * This is the main entry point for the phase. It should perform its specific task
     * (e.g., parse the source) and report any errors via the unit's error emitter.
     *
     * @param unit Pointer to the compilation unit to process.
     * @return `true` if the phase completed successfully; `false` if errors occurred.
     */
    virtual bool execute(class FrontendCompilationUnit *unit) = 0;

    /**
     * @brief Gets the name of the phase.
     *
     * Used primarily for debugging and logging purposes.
     *
     * @return A C-string representing the phase name (e.g., "ParsingPhase").
     */
    virtual const char *getName() = 0;

  protected:
    friend class FrontendCompilationUnit;
};

#endif // EZPACKER_FRONTENDCOMPILATIONUNITPHASE_H
