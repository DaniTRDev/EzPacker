#include "FrontendCompilerDriver.h"

FrontendCompilerDriver::FrontendCompilerDriver(const std::shared_ptr<ErrorCollector> &errorCollector,
                                               const std::shared_ptr<SourceManager> &sourceManager) :
    m_globalScope(std::make_shared<Scope>(nullptr, "@FrontendCompilerDriver@GlobalScope")),
    ErrorEmitter(errorCollector, sourceManager)
{
}

bool FrontendCompilerDriver::addSource(const std::string &sourceContent, const std::string &sourceName)
{
    FrontendCompilationUnit compilationUnit(getErrorCollector(), getSourceManager());
    if (!compilationUnit.create(sourceContent, sourceName))
    {
        emitError(ErrorSeverity::Fatal,
                  std::format("Failed to create compilation unit for source: {}", sourceName),
                  "FrontendCompilerDriver::addSource");
        return false;
    }
    m_compilationUnits.push_back(std::move(compilationUnit));

    return true;
}

bool FrontendCompilerDriver::compile()
{
    // First phase: Tokenization. TODO: Parallelize this phase in the future.
    if (!executeCompilationUnitPhase(std::make_shared<TokenizationPhase>()))
    {
        return false;
    }

    // Second phase: Parsing. TODO: Parallelize this phase in the future.
    if (!executeCompilationUnitPhase(std::make_shared<ParsingPhase>()))
    {
        return false;
    }

    // Third phase: Semantic analysis. TODO: Parallelize this phase in the future.
    if (!executeCompilationUnitPhase(std::make_shared<SemanticAnalysisPhase>()))
    {
        return false;
    }

    // Fourth phase: MIR generation (Lowering). TODO: Parallelize this phase in the future.
    if (!executeCompilationUnitPhase(std::make_shared<AstLoweringPhase>()))
    {
        return false;
    }

    return true;
}

bool FrontendCompilerDriver::executeCompilationUnitPhase(const std::shared_ptr<FrontendCompilationUnitPhase> &phase)
{
    if (!phase)
    {
        emitError(ErrorSeverity::Fatal,
                  "Invalid compilation unit phase",
                  "FrontendCompilerDriver::executeCompilationUnitPhase");
        return false;
    }

    for (auto &compilationUnit : m_compilationUnits)
    {
        if (!phase->execute(&compilationUnit))
        {
            emitError(ErrorSeverity::Fatal,
                      std::format("Failed to execute phase: {} for source: {}",
                                  phase->getName(),
                                  getSourceManager()->getSourceName(compilationUnit.getTargetSourceId())),
                      "FrontendCompilerDriver::executeCompilationUnitPhase");
            return false;
        }
    }

    return true;
}
