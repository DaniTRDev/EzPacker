#include "Views/MainMenuBar.h"
#include "Views/Editor.h"
#include "Views/FileExplorer.h"

const char *MainMenuBar::getName() { return "MainMenuBar"; }

void MainMenuBar::render()
{
    if (!ImGui::BeginMenuBar())
        return;

    // ── File ──────────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("New Tab", "Ctrl+T", false, m_editor != nullptr))
        {
            if (m_editor) m_editor->createNewTab();
        }

        if (ImGui::MenuItem("Open...", "Ctrl+O", false, m_editor != nullptr))
        {
            if (m_editor) m_editor->promptOpenFile();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Save", "Ctrl+S", false, m_editor != nullptr))
        {
            if (m_editor) m_editor->saveCurrentFile();
        }

        if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S", false, m_editor != nullptr))
        {
            if (m_editor) m_editor->promptSaveFileAs();
        }

        ImGui::Separator();
        ImGui::MenuItem("Exit", "Alt+F4", false, false);
        ImGui::EndMenu();
    }

    // ── Build ─────────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("Build"))
    {
        if (ImGui::MenuItem("Compile", "F5", false, m_editor != nullptr))
        {
            if (m_editor) m_editor->compileCurrentBuffer();
        }

        ImGui::Separator();
        ImGui::TextDisabled("Tokens, AST, Semantics and MIR");
        ImGui::TextDisabled("are shown in the side inspectors.");
        ImGui::EndMenu();
    }

    // ── View ──────────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("View"))
    {
        ImGui::MenuItem("File Explorer", nullptr, &m_showFileExplorer);
        ImGui::Separator();
        ImGui::TextDisabled("Inspector panels are always visible");
        ImGui::TextDisabled("in the Editor side-bar.");
        ImGui::EndMenu();
    }

    // ── Help ──────────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("Help"))
    {
        ImGui::TextDisabled("EZ Language Frontend IDE");
        ImGui::Separator();
        ImGui::BulletText("Native window: movable and resizable");
        ImGui::BulletText("Source editor + frontend pipeline inspector");
        ImGui::BulletText("Tokens, AST, symbols and MIR in one place");
        ImGui::EndMenu();
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|  EzFrontendDebugGUI  |  One Dark  |  JetBrains Mono");

    ImGui::EndMenuBar();
}
