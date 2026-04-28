#ifndef EZPACKER_COMPILER_SESSION_H
#define EZPACKER_COMPILER_SESSION_H

#include <string>
#include <vector>

namespace EzGui {

    struct TokenInfo {
        std::string type;
        std::string value;
        int line;
        int column;
    };

    struct AstNodeInfo {
        std::string name;
        std::string details;
        std::vector<AstNodeInfo> children;
    };

    struct MirBlockInfo {
        std::string label;
        std::vector<std::string> instructions;
    };

    struct LogMessage {
        int severity; // e.g. 0=Info, 1=Warning, 2=Error
        std::string module;
        std::string text;
        std::string location; // Optional source ref
    };

    class CompilerSession {
    public:
        virtual ~CompilerSession() = default;

        // Fetch lexer state
        virtual std::vector<TokenInfo> GetTokens() const = 0;

        // Fetch AST state
        virtual AstNodeInfo GetAstRoot() const = 0;

        // Fetch MIR state
        virtual std::vector<MirBlockInfo> GetMirBlocks() const = 0;

        // Fetch Analysis state
        virtual std::string GetMirAnalysisText() const = 0;

        // Fetch Lowerer/Backend state
        virtual std::vector<std::string> GetLoweredInstructions() const = 0;
        
        // Fetch logs (info, warnings, errors from ErrorCollector/ErrorEmitter)
        virtual std::vector<LogMessage> GetLogs() const = 0;

        // Triggers a compile step on some source code
        virtual void CompileSource(const std::string& source) = 0;
    };

    class DummyCompilerSession : public CompilerSession {
    public:
        std::vector<TokenInfo> GetTokens() const override;
        AstNodeInfo GetAstRoot() const override;
        std::vector<MirBlockInfo> GetMirBlocks() const override;
        std::string GetMirAnalysisText() const override;
        std::vector<std::string> GetLoweredInstructions() const override;
        std::vector<LogMessage> GetLogs() const override;
        void CompileSource(const std::string& source) override;
    };

} // namespace EzGui

#endif // EZPACKER_COMPILER_SESSION_H
