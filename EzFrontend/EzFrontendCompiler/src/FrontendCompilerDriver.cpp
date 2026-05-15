#include "FrontendCompilerDriver.h"

FrontendCompilerDriver::FrontendCompilerDriver(const std::shared_ptr<ErrorCollector> &errorCollector,
                                               const std::shared_ptr<SourceManager> &sourceManager) :
    m_globalScope(std::make_shared<Scope>(nullptr, "@FrontendCompilerDriver@GlobalScope")),
    ErrorEmitter(errorCollector, sourceManager)
{
}

bool FrontendCompilerDriver::addSource(const std::string &sourceContent,
                                       const std::string &sourceName,
                                       std::shared_ptr<FrontendCompilationUnit> *outUnit)
{
    size_t id = getSourceManager()->addSourceContent(sourceName, sourceContent);
    if (id == 0)
    {
        emitError(ErrorSeverity::Warning,
                  "Source has already been added, skipping. Source name: " + sourceName,
                  "FrontendCompilerDriver::addSource");
        return true;
    }

    auto compilationUnit = std::make_shared<FrontendCompilationUnit>(getErrorCollector(), getSourceManager());
    if (!compilationUnit->create(id))
    {
        emitError(ErrorSeverity::Fatal,
                  std::format("Failed to create compilation unit for source: {}", sourceName),
                  "FrontendCompilerDriver::addSource");
        return false;
    }

    if (outUnit)
    {
        *outUnit = compilationUnit;
    }

    m_queuedCompilationUnits.push(std::move(compilationUnit));

    return true;
}

bool FrontendCompilerDriver::compile()
{
    std::queue<std::shared_ptr<FrontendCompilationUnit>> workingSet = m_queuedCompilationUnits;
    while (!workingSet.empty())
    {
        std::shared_ptr<FrontendCompilationUnit> unit = std::move(workingSet.front());
        std::shared_ptr<IncludePhase> includePhase = std::make_shared<IncludePhase>();

        workingSet.pop();

        if (!executeCompilationUnitPhase(unit.get(), std::make_shared<TokenizationPhase>()) ||
            !executeCompilationUnitPhase(unit.get(), std::make_shared<ParsingPhase>()) ||
            !executeCompilationUnitPhase(unit.get(), includePhase))
        {
            return false;
        }

        std::set<std::string_view> includedFiles;
        includePhase->moveIncludedFilesToDest(includedFiles);

        for (const std::string_view &includedFile : includedFiles)
        {
            std::shared_ptr<FrontendCompilationUnit> includedUnit;
            if (!addSourceFromFile(std::string(includedFile), &includedUnit))
            {
                return false;
            }

            if (includedUnit)
            {
                // The compilation unit was created and added to the queue in addSourceFromFile, so we can just push it
                // to the working set. Because of the way addSourceFromFile works, we can assume includedUnit is not
                // duplicated, as it will return nullptr if the source has already been added.
                workingSet.push(includedUnit);
            }
        }
    }

    // At this point, all files and their respective included files have been tokenized and parsed. Now we can proceed
    // with the next phases for each compilation unit.

    /**
     * Important: the order of these visitors is important. Instead of traversing the AST in a single loop and executing
     * the three visitors, we are delaying it to two separate loops. This allows forward declarations to work
     * correctly, as the definition visitor will populate the symbol table with all symbols before the resolution and
     * type checking visitors run. If we were to run all three visitors in a single loop, we would encounter issues with
     * forward declarations, as the resolution and type checking visitors would not be able to find symbols that have
     * not been defined yet.
     *
     */

    workingSet = m_queuedCompilationUnits;
    while (!workingSet.empty())
    {
        std::shared_ptr<FrontendCompilationUnit> unit = std::move(workingSet.front());
        workingSet.pop();

        const std::shared_ptr<ErrorCollector> &errorCollector = unit->getErrorCollector();
        const std::shared_ptr<SourceManager> &sourceManager = unit->getSourceManager();
        std::shared_ptr<BasicSemanticContext> semanticContext =
                std::make_shared<BasicSemanticContext>(errorCollector, sourceManager, unit->getGlobalScope());
        TypedPoolLinkedList<AstNode> *globalScopeAstNodes = unit->getGlobalScopeAstNodes();

        unit->setSemanticContext(semanticContext);
        unit->setGlobalScopeAstNodes(globalScopeAstNodes);

        if (!executeCompilationUnitPhase(unit.get(), std::make_shared<SymbolDefinitionPhase>()))
        {
            return false;
        }

        Symbol *outErrSym = nullptr;
        std::map<std::string_view, Symbol *> unitSymbolTable = unit->getGlobalScope()->getSymbols();

        if (!m_globalScope->mergeSymbols(unitSymbolTable, &outErrSym))
        {
            emitError(ErrorSeverity::Fatal,
                      std::format("Could not merge symbol '{}' of '{}' into global scope",
                                  outErrSym->getName(),
                                  getSourceManager()->getSourceName(unit->getTargetSourceId())),
                      "FrontendCompilerDriver::compile");
            return false;
        }

        unit->getGlobalScope()->setParent(m_globalScope.get());
    }

    workingSet = m_queuedCompilationUnits;
    while (!workingSet.empty())
    {
        std::shared_ptr<FrontendCompilationUnit> unit = std::move(workingSet.front());
        workingSet.pop();

        if (!executeCompilationUnitPhase(unit.get(), std::make_shared<SymbolAndTypeResolverPhase>()))
        {
            return false;
        }

        if (!executeCompilationUnitPhase(unit.get(), std::make_shared<TypeCheckPhase>()))
        {
            return false;
        }

        if (!executeCompilationUnitPhase(unit.get(), std::make_shared<AstLoweringPhase>()))
        {
            return false;
        }
    }
    return true;
}

bool FrontendCompilerDriver::addSourceFromFile(const std::string &filePath,
                                               std::shared_ptr<FrontendCompilationUnit> *outUnit)
{
    std::filesystem::path path = getSourceManager()->resolveSourcePath(filePath);
    std::string fileName = path.filename().string();

    if (getSourceManager()->doesSourceNameExist(fileName))
    {
        emitError(ErrorSeverity::Warning,
                  std::format("Source file already included: {}", filePath),
                  "FrontendCompilerDriver::addSourceFromFile");
        return true;
    }

    if (!std::filesystem::exists(path))
    {
        emitError(ErrorSeverity::Fatal,
                  std::format("Source file does not exist: {}", filePath),
                  "FrontendCompilerDriver::addSourceFromFile");
        return false;
    }

    if (!std::filesystem::is_regular_file(path))
    {
        emitError(ErrorSeverity::Fatal,
                  std::format("Source file is not a regular file: {}", filePath),
                  "FrontendCompilerDriver::addSourceFromFile");
        return false;
    }

    std::ifstream fileStream(path, std::ios::binary | std::ios::ate);
    if (!fileStream.is_open())
    {
        emitError(ErrorSeverity::Fatal,
                  std::format("Failed to open source file: {}", filePath),
                  "FrontendCompilerDriver::addSourceFromFile");
        return false;
    }

    std::streamsize fileSize = fileStream.tellg();
    fileStream.seekg(0, std::ios::beg);

    std::string sourceContent;
    sourceContent.resize(fileSize);

    if (!fileStream.read(sourceContent.data(), fileSize))
    {
        emitError(ErrorSeverity::Fatal,
                  std::format("Failed to read source file: {}", filePath),
                  "FrontendCompilerDriver::addSourceFromFile");
        return false;
    }

    return addSource(sourceContent, filePath, outUnit);
}

bool FrontendCompilerDriver::executeCompilationUnitPhase(FrontendCompilationUnit *unit,
                                                         const std::shared_ptr<FrontendCompilationUnitPhase> &phase)
{
    if (!phase)
    {
        emitError(ErrorSeverity::Fatal,
                  "Invalid compilation unit phase",
                  "FrontendCompilerDriver::executeCompilationUnitPhase");
        return false;
    }

    if (!phase->execute(unit))
    {
        emitError(ErrorSeverity::Fatal,
                  std::format("Failed to execute phase: {} for source: {}",
                              phase->getName(),
                              getSourceManager()->getSourceName(unit->getTargetSourceId())),
                  "FrontendCompilerDriver::executeCompilationUnitPhase");
        return false;
    }

    return true;
}
