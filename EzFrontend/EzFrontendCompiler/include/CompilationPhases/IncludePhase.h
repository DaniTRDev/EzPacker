/**
 * @file IncludePhase.h
 * @brief Compilation phase for resolving include directives.
 *
 * The IncludePhase scans the parsed AST for `IncludeAstNode`s and resolves the
 * included files. This phase is crucial for building the dependency graph of
 * source files and ensuring all necessary code is compiled.
 */
#ifndef EZPACKER_INCLUDEPHASE_H
#define EZPACKER_INCLUDEPHASE_H

#include "EzFrontendCompilerCommon.h"
#include "FrontendCompilationUnitPhase.h"
#include "IncludeVisitor/IncludeVisitor.h"

/**
 * @class IncludePhase
 * @brief Resolves file inclusions in the AST.
 *
 * This phase uses the `IncludeVisitor` to traverse the AST and collect all
 * included file paths. These paths are then made available to the compiler
 * driver so that the corresponding files can be loaded and compiled.
 */
class IncludePhase : public FrontendCompilationUnitPhase
{
  public:
    /**
     * @brief Executes the include resolution phase.
     *
     * Runs the `IncludeVisitor` on the unit's AST to find include directives.
     *
     * @param unit Pointer to the compilation unit to process.
     * @return `true` if include resolution succeeded; `false` otherwise.
     */
    bool execute(class FrontendCompilationUnit *unit) override;

    /**
     * @brief Gets the phase name.
     *
     * @return "IncludePhase"
     */
    const char *getName() override;

    /**
     * @brief Retrieves the set of included files discovered during this phase.
     *
     * Moves the internal set of discovered file paths to the destination set.
     * This is typically called by the compiler driver after the phase completes.
     *
     * @param[out] dest The set to receive the included file paths.
     */
    void moveIncludedFilesToDest(std::set<std::string_view> &dest);

  private:
    std::set<std::string_view> m_includedFiles;
};

#endif // EZPACKER_INCLUDEPHASE_H
