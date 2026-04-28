#include "EzGuiApp.h"
#include <iostream>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <GLFW/glfw3.h> // Will drag in OpenGL headers

#include "RealCompilerSession.h"

namespace EzGui {

    static void GlfwErrorCallback(int error, const char* description) {
        std::cerr << "GLFW Error " << error << ": " << description << std::endl;
    }

    EzGuiApp::EzGuiApp()
        : m_Window(nullptr)
        , m_Session(new RealCompilerSession())
        , m_CurrentArch(Architecture::x64)
        , m_ShowLexer(true)
        , m_ShowAst(true)
        , m_ShowMir(true)
        , m_ShowMirAnalysis(true)
        , m_ShowMirLowerer(true)
        , m_ShowLog(true)
        , m_ShowEditor(true) {
        
        const char* defaultCode = "i64 main() {\n  ret; \n}\n";
        strncpy(m_EditorBuffer, defaultCode, sizeof(m_EditorBuffer) - 1);
    }

    EzGuiApp::~EzGuiApp() {
        Shutdown();
    }

    bool EzGuiApp::Initialize(const std::string& title, int width, int height) {
        // Setup window
        glfwSetErrorCallback(GlfwErrorCallback);
        if (!glfwInit())
            return false;

        // GL 3.0 + GLSL 130
        const char* glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

        // Create window with graphics context
        GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (window == nullptr) {
            glfwTerminate();
            return false;
        }
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable vsync
        m_Window = window;

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

        return true;
    }

    void EzGuiApp::Run() {
        GLFWwindow* window = static_cast<GLFWwindow*>(m_Window);
        if (!window) return;

        ImGuiIO& io = ImGui::GetIO();

        // Main loop
        while (!glfwWindowShouldClose(window)) {
            // Poll and handle events
            glfwPollEvents();

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            
            // Enable fullscreen dockspace
            ImGui::DockSpaceOverViewport();

            RenderFrame();

            // Rendering
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            
            // Update and Render additional Platform Windows
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                GLFWwindow* backup_current_context = glfwGetCurrentContext();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                glfwMakeContextCurrent(backup_current_context);
            }

            glfwSwapBuffers(window);
        }
    }

    void EzGuiApp::Shutdown() {
        if (!m_Window) return;
        
        delete m_Session;

        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(static_cast<GLFWwindow*>(m_Window));
        glfwTerminate();
        m_Window = nullptr;
    }

    void EzGuiApp::RenderFrame() {
        DrawMenuBar();

        if (m_ShowEditor) {
            ImGui::Begin("Source Code Editor", &m_ShowEditor);
            if (ImGui::Button("Compile")) {
                m_Session->CompileSource(m_EditorBuffer);
            }
            ImGui::InputTextMultiline("##source", m_EditorBuffer, sizeof(m_EditorBuffer),
                ImVec2(-FLT_MIN, -ImGui::GetTextLineHeight() * 2), ImGuiInputTextFlags_AllowTabInput);
            ImGui::End();
        }

        DrawFrontendPanels();
        DrawBackendPanels();
        
        if (m_ShowLog) {
            ImGui::Begin("Compiler Log (ErrorEmitter Output)", &m_ShowLog);
            if (ImGui::BeginTable("LogTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
                ImGui::TableSetupColumn("Severity", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                ImGui::TableSetupColumn("Module", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Location", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                for (const auto& log : m_Session->GetLogs()) {
                    ImGui::TableNextRow();
                    
                    ImGui::TableNextColumn();
                    switch(log.severity) {
                        case 0: ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "INFO"); break;
                        case 1: ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "WARNING"); break;
                        default: ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "ERROR"); break;
                    }

                    ImGui::TableNextColumn();
                    ImGui::Text("%s", log.module.c_str());

                    ImGui::TableNextColumn();
                    ImGui::Text("%s", log.location.c_str());

                    ImGui::TableNextColumn();
                    ImGui::TextWrapped("%s", log.text.c_str());
                }
                ImGui::EndTable();
            }
            ImGui::End();
        }
    }

    void EzGuiApp::DrawMenuBar() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit")) {
                    glfwSetWindowShouldClose(static_cast<GLFWwindow*>(m_Window), GLFW_TRUE);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Source Editor", nullptr, &m_ShowEditor);
                ImGui::Separator();
                ImGui::MenuItem("Lexer Tokens", nullptr, &m_ShowLexer);
                ImGui::MenuItem("AST Overview", nullptr, &m_ShowAst);
                ImGui::MenuItem("MIR Inspector", nullptr, &m_ShowMir);
                ImGui::Separator();
                ImGui::MenuItem("MIR Analysis", nullptr, &m_ShowMirAnalysis);
                ImGui::MenuItem("MIR Lowering", nullptr, &m_ShowMirLowerer);
                ImGui::Separator();
                ImGui::MenuItem("Compiler Log", nullptr, &m_ShowLog);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Architecture")) {
                // Architecture switch: defaults to x64
                if (ImGui::MenuItem("x64 (Default)", nullptr, m_CurrentArch == Architecture::x64))
                    m_CurrentArch = Architecture::x64;
                if (ImGui::MenuItem("x86", nullptr, m_CurrentArch == Architecture::x86))
                    m_CurrentArch = Architecture::x86;
                if (ImGui::MenuItem("ARM", nullptr, m_CurrentArch == Architecture::ARM))
                    m_CurrentArch = Architecture::ARM;
                if (ImGui::MenuItem("ARM64", nullptr, m_CurrentArch == Architecture::ARM64))
                    m_CurrentArch = Architecture::ARM64;
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }

    void EzGuiApp::DrawFrontendPanels() {
        if (m_ShowLexer) {
            ImGui::Begin("EzLexer: Tokens", &m_ShowLexer);
            ImGui::Text("List of source tokens:");
            if (ImGui::BeginTable("TokensTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Line");
                ImGui::TableSetupColumn("Col");
                ImGui::TableSetupColumn("Type");
                ImGui::TableSetupColumn("Value");
                ImGui::TableHeadersRow();

                for (const auto& t : m_Session->GetTokens()) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn(); ImGui::Text("%d", t.line);
                    ImGui::TableNextColumn(); ImGui::Text("%d", t.column);
                    ImGui::TableNextColumn(); ImGui::Text("%s", t.type.c_str());
                    ImGui::TableNextColumn(); ImGui::Text("%s", t.value.c_str());
                }
                ImGui::EndTable();
            }
            ImGui::End();
        }

        if (m_ShowAst) {
            ImGui::Begin("EzAstLowerer: AST View", &m_ShowAst);
            ImGui::Text("Abstract Syntax Tree visualization:");
            
            auto root = m_Session->GetAstRoot();
            DrawAstNode(root);

            ImGui::End();
        }

        if (m_ShowMir) {
            ImGui::Begin("EzMir: Internal Representation", &m_ShowMir);
            ImGui::Text("Mid-level IR blocks:");
            
            for (const auto& block : m_Session->GetMirBlocks()) {
                if (ImGui::TreeNode(block.label.c_str())) {
                    for (const auto& inst : block.instructions) {
                        ImGui::Text("  %s", inst.c_str());
                    }
                    ImGui::TreePop();
                }
            }
            
            ImGui::End();
        }
    }

    void EzGuiApp::DrawBackendPanels() {
        if (m_ShowMirAnalysis) {
            ImGui::Begin("EzMirAnalysis", &m_ShowMirAnalysis);
            ImGui::TextWrapped("%s", m_Session->GetMirAnalysisText().c_str());
            ImGui::End();
        }

        if (m_ShowMirLowerer) {
            ImGui::Begin("EzMirLowerer -> Target Config", &m_ShowMirLowerer);
            ImGui::Text("Current ABI Target: ");
            ImGui::SameLine();
            switch (m_CurrentArch) {
                case Architecture::x64: ImGui::Text("x64"); break;
                case Architecture::x86: ImGui::Text("x86"); break;
                case Architecture::ARM: ImGui::Text("ARM"); break;
                case Architecture::ARM64: ImGui::Text("ARM64"); break;
            }
            ImGui::Separator();
            ImGui::Text("Lowered Assembly instructions:");
            for (const auto& asmLine : m_Session->GetLoweredInstructions()) {
                ImGui::Text("%s", asmLine.c_str());
            }
            ImGui::End();
        }
    }

    void EzGuiApp::DrawAstNode(const AstNodeInfo& node) {
        if (ImGui::TreeNodeEx((node.name + " (" + node.details + ")").c_str(),
                              node.children.empty() ? ImGuiTreeNodeFlags_Leaf : ImGuiTreeNodeFlags_None)) {
            for (const auto& child : node.children) {
                DrawAstNode(child);
            }
            ImGui::TreePop();
        }
    }

} // namespace EzGui
