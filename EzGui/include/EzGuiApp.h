#ifndef EZPACKER_EZGUIAPP_H
#define EZPACKER_EZGUIAPP_H

#include <string>

namespace EzGui {

    enum class Architecture {
        x64,
        x86,
        ARM,
        ARM64
    };

    class CompilerSession;
    struct AstNodeInfo;

    class EzGuiApp {
    public:
        EzGuiApp();
        ~EzGuiApp();

        bool Initialize(const std::string& title, int width, int height);
        void Run();
        void Shutdown();

    private:
        void RenderFrame();
        void DrawMenuBar();
        void DrawFrontendPanels();
        void DrawBackendPanels();
        void DrawAstNode(const AstNodeInfo& node);
        
        // Window handle (opaque pointer to GLFWwindow to avoid exposing GLFW headers here)
        void* m_Window;

        CompilerSession* m_Session;

        // State
        Architecture m_CurrentArch;
        bool m_ShowLexer;
        bool m_ShowAst;
        bool m_ShowMir;
        bool m_ShowMirAnalysis;
        bool m_ShowMirLowerer;
        bool m_ShowLog;
        bool m_ShowEditor;

        char m_EditorBuffer[8192];
    };

} // namespace EzGui

#endif // EZPACKER_EZGUIAPP_H

