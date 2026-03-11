/**
 * @file EzFrontendWrapper.h
 * @brief Convenience wrapper that drives the frontend pipeline from the
 *        debug GUI: open a file, tokenize it, parse it, and expose results.
 *
 * EzFrontendWrapper is used by the Editor view to interactively compile
 * source files.  It owns the tokenizer, parsing context, error collector,
 * and source manager, exposing the token list and AST parse results for
 * the GUI views to render.
 */
#ifndef EZPACKER_EZFRONTENDWRAPPER_H
#define EZPACKER_EZFRONTENDWRAPPER_H

#include "EzFrontendDebugGUICommon.h"

struct FrontendDiagnosticEntry
{
    ErrorSeverity m_severity{ ErrorSeverity::NoError };
    SourceReference m_sourceRef{};
    std::string m_message;
    std::string m_sender;
    std::string m_timeStamp;
};

struct FrontendPipelineStage
{
    std::string m_name;
    bool m_available{ false };
    std::string m_detail;
};

class EzFrontendWrapper
{
  public:
    explicit EzFrontendWrapper(std::shared_ptr<SourceLoggingSink> sink);

    bool loadSourceFromFile(const std::filesystem::path &filePath);
    bool saveSourceToFile(const std::filesystem::path &filePath, std::string_view sourceText) const;
    bool compileSource(const std::string &sourceText,
                       const std::string &sourceName,
                       const std::filesystem::path &workingDirectory);

    const std::shared_ptr<ErrorCollector> &getErrorCollector() const;
    const std::shared_ptr<SourceLoggingSink> &getLoggingSink() const;
    const std::shared_ptr<SourceManager> &getSourceManager() const;
    const std::shared_ptr<FrontendCompilationUnit> &getCompilationUnit() const;

    const std::vector<TokenInformation> &getTokens() const;
    const std::vector<AstNode *> &getParseResult() const;
    const std::vector<FrontendDiagnosticEntry> &getDiagnostics() const;
    const std::vector<std::string> &getIncludedFiles() const;
    const std::vector<FrontendPipelineStage> &getPipelineStages() const;

    const std::filesystem::path &getLoadedFilePath() const;
    const std::string &getLoadedSourceText() const;
    const std::string &getSourceName() const;
    const std::filesystem::path &getWorkingDirectory() const;

    bool hasCompilationResult() const;
    bool lastCompilationSucceeded() const;

  private:
    static void onDiagnostic(void *userParam, const std::shared_ptr<Error> &error);
    void rebuildFrontendState(const std::filesystem::path &workingDirectory);
    void clearCompilationArtifacts();
    void collectIncludeFiles();
    void rebuildPipelineStages();

  private:
    std::shared_ptr<ErrorCollector> m_errorCollector;
    std::shared_ptr<SourceLoggingSink> m_sink;
    std::shared_ptr<SourceManager> m_sourceManager;
    std::shared_ptr<FrontendCompilerDriver> m_compilerDriver;
    std::shared_ptr<FrontendCompilationUnit> m_compilationUnit;

    std::filesystem::path m_loadedFilePath;
    std::filesystem::path m_workingDirectory;
    std::string m_sourceName;
    std::string m_sourceText;
    bool m_lastCompilationSucceeded{ false };

    mutable std::vector<AstNode *> m_cachedParseResult;
    std::vector<FrontendDiagnosticEntry> m_diagnostics;
    std::vector<std::string> m_includedFiles;
    std::vector<FrontendPipelineStage> m_pipelineStages;
};

#endif // EZPACKER_EZFRONTENDWRAPPER_H
