#ifndef EZPACKER_COMPILERDRIVER_H
#define EZPACKER_COMPILERDRIVER_H

#include "EzFrontendCompilerCommon.h"
#include "FrontendCompilationUnit.h"
#include "FrontendCompilationUnitPhase.h"
#include "CompilationPhases/AstLowering.h"
#include "CompilationPhases/Parsing.h"
#include "CompilationPhases/SemanticAnalysis.h"
#include "CompilationPhases/Tokenization.h"

class FrontendCompilerDriver : public ErrorEmitter
{
  public:
    /**
     * Constructs a new FrontendCompilerDriver with the given error collector and source manager.
     * @param errorCollector
     * @param sourceManager
     */
    FrontendCompilerDriver(const std::shared_ptr<ErrorCollector> &errorCollector,
                           const std::shared_ptr<SourceManager> &sourceManager);

    /**
     * Creates a compilation unit for the given source content and source name, and adds it to the list of compilation
     * units. The method returns true if the source was successfully added, and false if there was an error during the
     * process.
     * @param sourceContent
     * @param sourceName
     * @return bool
     */
    bool addSource(const std::string &sourceContent, const std::string &sourceName);

    /**
     * Starts the compilation process for all added sources. This method will process each source, generate the
     * corresponding compilation units, and perform the necessary steps to compile the sources. If any errors are
     * encountered during compilation, they will be collected and emitted through the error collector. The method
     * returns true if the compilation process completes successfully without any errors, and false otherwise.
     * @return bool
     */
    bool compile();

  private:
    /**
     * Executes the specified compilation unit phase for all compilation units. This method will iterate through each
     * compilation unit and perform the given phase (e.g., tokenization, parsing) on it. If any errors are encountered
     * during the execution of the phase, they will be collected and emitted through the error collector. The method
     * returns true if the phase is successfully executed for all compilation units without any errors, and false
     * otherwise.
     * @param phase
     * @return bool
     */
    bool executeCompilationUnitPhase(const std::shared_ptr<FrontendCompilationUnitPhase> &phase);

  private:
    std::shared_ptr<Scope> m_globalScope; // Scope shared across all compilation units.
    std::list<FrontendCompilationUnit> m_compilationUnits;
};

#endif // EZPACKER_COMPILERDRIVER_H
