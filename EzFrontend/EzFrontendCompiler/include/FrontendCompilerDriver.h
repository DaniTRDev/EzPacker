/**
 * @file FrontendCompilerDriver.h
 * @brief Main driver class for the frontend compilation process.
 *
 * The FrontendCompilerDriver orchestrates the entire compilation pipeline for EzPacker.
 * It manages the lifecycle of compilation units, coordinates the execution of various
 * compilation phases (tokenization, parsing, semantic analysis, etc.), and handles
 * error reporting.
 *
 * Key responsibilities:
 * - Managing source files and creating `FrontendCompilationUnit`s.
 * - Scheduling and running compilation phases in the correct order.
 * - Maintaining global state such as the global scope.
 * - Collecting and emitting errors via `ErrorCollector`.
 */
#ifndef EZPACKER_COMPILERDRIVER_H
#define EZPACKER_COMPILERDRIVER_H

#include "EzFrontendCompilerCommon.h"
#include "FrontendCompilationUnit.h"
#include "FrontendCompilationUnitPhase.h"
#include "CompilationPhases/AstLowering.h"
#include "CompilationPhases/IncludePhase.h"
#include "CompilationPhases/Parsing.h"
#include "CompilationPhases/SemanticAnalysis.h"
#include "CompilationPhases/Tokenization.h"

/**
 * @class FrontendCompilerDriver
 * @brief Orchestrates the compilation of source files into an intermediate representation.
 *
 * This class acts as the central controller for the frontend. It takes source code (either
 * from files or strings), creates compilation units, and drives them through the necessary
 * phases to produce a valid AST and eventually MIR (Mid-level Intermediate Representation).
 *
 * It maintains a queue of compilation units to process, handling dependencies (like included files)
 * as they are discovered.
 */
class FrontendCompilerDriver : public ErrorEmitter
{
  public:
    /**
     * @brief Constructs a new FrontendCompilerDriver.
     *
     * Initializes the driver with the necessary error handling and source management components.
     *
     * @param errorCollector Shared pointer to the error collector for reporting diagnostics.
     * @param sourceManager Shared pointer to the source manager for handling source files.
     */
    FrontendCompilerDriver(const std::shared_ptr<ErrorCollector> &errorCollector,
                           const std::shared_ptr<SourceManager> &sourceManager);

    /**
     * @brief Adds a source string to the compilation process.
     *
     * Creates a new `FrontendCompilationUnit` for the provided source content and adds it to the
     * processing queue.
     *
     * @param sourceContent The raw source code as a string.
     * @param sourceName A name for the source (e.g., "main.ez" or "<stdin>"), used in diagnostics.
     * @param[out] outUnit Optional pointer to receive the created compilation unit.
     * @return `true` if the source was successfully added; `false` otherwise.
     */
    bool addSource(const std::string &sourceContent,
                   const std::string &sourceName,
                   std::shared_ptr<FrontendCompilationUnit> *outUnit = nullptr);

    /**
     * @brief Adds a source file to the compilation process.
     *
     * Reads the content of the specified file, creates a `FrontendCompilationUnit`, and adds it
     * to the processing queue.
     *
     * @param filePath The absolute or relative path to the source file.
     * @param[out] outUnit Optional pointer to receive the created compilation unit.
     * @return `true` if the file was successfully read and added; `false` otherwise.
     */
    bool addSourceFromFile(const std::string &filePath, std::shared_ptr<FrontendCompilationUnit> *outUnit = nullptr);

    /**
     * @brief Executes the compilation pipeline.
     *
     * Processes all queued compilation units through their required phases. This includes tokenization,
     * parsing, include resolution, semantic analysis, and AST lowering. If new units are discovered
     * (e.g., via includes), they are added to the queue and processed as well.
     *
     * @return `true` if all units compiled successfully without errors; `false` otherwise.
     */
    bool compile();

  private:
    /**
     * @brief Runs a specific phase on a compilation unit.
     *
     * Helper method to execute a single phase (e.g., parsing) on a given unit and handle any
     * resulting errors.
     *
     * @param unit Pointer to the compilation unit to process.
     * @param phase Shared pointer to the phase to execute.
     * @return `true` if the phase completed successfully; `false` otherwise.
     */
    bool executeCompilationUnitPhase(FrontendCompilationUnit *unit,
                                     const std::shared_ptr<FrontendCompilationUnitPhase> &phase);

  private:
    /**
     * @brief The global scope shared across all compilation units.
     *
     * This scope contains top-level declarations that are visible across file boundaries.
     */
    std::shared_ptr<Scope> m_globalScope;

    /**
     * @brief Queue of compilation units waiting to be processed.
     *
     * Units are processed in FIFO order (queue). New units discovered via includes are pushed
     * onto this queue.
     */
    std::queue<std::shared_ptr<FrontendCompilationUnit>> m_queuedCompilationUnits;
};

#endif // EZPACKER_COMPILERDRIVER_H
