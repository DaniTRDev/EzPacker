#ifndef EZPACKER_REAL_COMPILER_SESSION_H
#define EZPACKER_REAL_COMPILER_SESSION_H

#include "CompilerSession.h"
#include <memory>
#include <vector>

class FrontendCompilerDriver;
class FrontendCompilationUnit;
class ErrorCollector;
class SourceManager;
struct Error;

namespace EzGui {

    class RealCompilerSession : public CompilerSession {
    public:
        RealCompilerSession();
        ~RealCompilerSession() override;

        std::vector<TokenInfo> GetTokens() const override;
        AstNodeInfo GetAstRoot() const override;
        std::vector<MirBlockInfo> GetMirBlocks() const override;
        std::string GetMirAnalysisText() const override;
        std::vector<std::string> GetLoweredInstructions() const override;
        std::vector<LogMessage> GetLogs() const override;

        void CompileSource(const std::string& source) override;

        // Callback for ErrorCollector
        static void OnCompilerError(void* userParam, const std::shared_ptr<Error>& error);

    private:
        std::shared_ptr<ErrorCollector> m_errorCollector;
        std::shared_ptr<SourceManager> m_sourceManager;
        std::unique_ptr<FrontendCompilerDriver> m_driver;
        std::shared_ptr<FrontendCompilationUnit> m_mainUnit;
        std::vector<LogMessage> m_logs;
        std::string m_lastSource;

        void BuildAstInfo(AstNodeInfo& info, class AstNode* node) const;
    };

} // namespace EzGui

#endif // EZPACKER_REAL_COMPILER_SESSION_H
