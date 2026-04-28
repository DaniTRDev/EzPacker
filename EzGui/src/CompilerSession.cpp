#include "CompilerSession.h"

namespace EzGui {

    std::vector<TokenInfo> DummyCompilerSession::GetTokens() const {
        return {
            {"Keyword", "while", 1, 1},
            {"Identifier", "true", 1, 7},
            {"Punctuation", "{", 1, 12},
            {"Identifier", "do_something", 2, 5},
            {"Punctuation", "}", 3, 1}
        };
    }

    AstNodeInfo DummyCompilerSession::GetAstRoot() const {
        AstNodeInfo root {"Module", "Root node", {}};
        AstNodeInfo loop {"WhileAstNode", "Line 1", {}};
        loop.children.push_back({"ConditionAstNode", "true", {}});
        loop.children.push_back({"Instruction", "do_something", {}});
        root.children.push_back(loop);
        return root;
    }

    std::vector<MirBlockInfo> DummyCompilerSession::GetMirBlocks() const {
        return {
            {"bb0", {"%1 = load true", "br %1, bb1, bb2"}},
            {"bb1", {"call do_something", "jmp bb0"}},
            {"bb2", {"ret"}}
        };
    }

    std::string DummyCompilerSession::GetMirAnalysisText() const {
        return "CFG Analysis:\nbb0 -> bb1, bb2\nbb1 -> bb0\nDominators: ...";
    }

    std::vector<std::string> DummyCompilerSession::GetLoweredInstructions() const {
        return {
            "LBB0:",
            "  mov eax, 1",
            "  test eax, eax",
            "  jz LBB2",
            "LBB1:",
            "  call do_something",
            "  jmp LBB0",
            "LBB2:",
            "  ret"
        };
    }

    std::vector<LogMessage> DummyCompilerSession::GetLogs() const {
        return {
            { 0, "EzLexer", "Started tokenization", "" },
            { 1, "EzAstLowerer", "Variable not initialized before use", "main.ez:5" },
            { 2, "EzMir", "Instruction has incompatible operands", "main.ez:12" }
        };
    }

    void DummyCompilerSession::CompileSource(const std::string& source) {
        // Dummy implementation. Will update state when wired up to EzPacker
    }

} // namespace EzGui
