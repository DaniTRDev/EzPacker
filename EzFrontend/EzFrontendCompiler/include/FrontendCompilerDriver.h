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
     * process. If the source is successfully added and the outUnit parameter is provided, the created compilation unit
     * will be assigned to the outUnit pointer for further use.
     * @param sourceContent
     * @param sourceName
     * @param outUnit
     * @return bool
     */
    bool addSource(const std::string &sourceContent,
                   const std::string &sourceName,
                   std::shared_ptr<FrontendCompilationUnit> *outUnit = nullptr);

    /**
     * Adds a source file to the compilation process by creating a compilation unit for the file's content. The method
     * reads the content of the specified file, creates a compilation unit for it, and adds it to the list of
     * compilation units. The method returns true if the file was successfully added, and false if there was an error
     * during the process (e.g., if the file could not be read). If the file is successfully added and the outUnit
     * parameter is provided, the created compilation unit will be assigned to the outUnit pointer for further use.
     * @param filePath
     * @param outUnit
     * @return bool
     */
    bool addSourceFromFile(const std::string &filePath, std::shared_ptr<FrontendCompilationUnit> *outUnit = nullptr);

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
     * Executes the specified compilation unit phase for the given compilation unit. If any errors are encountered
     * during the execution of the phase, they will be collected and emitted through the error collector. The method
     * returns true if the phase is successfully executed, and false otherwise.
     * @param unit
     * @param phase
     * @return bool
     */
    bool executeCompilationUnitPhase(FrontendCompilationUnit *unit,
                                     const std::shared_ptr<FrontendCompilationUnitPhase> &phase);

  private:
    std::shared_ptr<Scope> m_globalScope; // Scope shared across all compilation units.
    std::stack<std::shared_ptr<FrontendCompilationUnit>>
            m_queuedCompilationUnits; /*
                                       * Compilation units that are waiting to be processed in the compilation. The
                                       * top of the stack is the next unit to be processed.
                                       */
};

#endif // EZPACKER_COMPILERDRIVER_H
