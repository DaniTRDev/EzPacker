#include "Views/MainMenuBar.h"

const char *MainMenuBar::getName() { return "MainMenuBar"; }

void MainMenuBar::render()
{
    if (!ImGui::BeginMenuBar())
    {
        return;
    }

    if (ImGui::BeginMenu("File"))
    {
        ImGui::MenuItem("Open...", "Ctrl+O", false, false);
        ImGui::MenuItem("Save", "Ctrl+S", false, false);
        ImGui::MenuItem("Save As...", "Ctrl+Shift+S", false, false);
        ImGui::Separator();
        ImGui::MenuItem("Close", nullptr, false, false);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Build"))
    {
        ImGui::MenuItem("Compile current buffer", "F5", false, false);
        ImGui::MenuItem("Re-run frontend pipeline", nullptr, false, false);
        ImGui::Separator();
        ImGui::TextDisabled("Lexer, AST, Semantics and MIR are visible in the side inspectors.");
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
        ImGui::MenuItem("Project panel", nullptr, true, false);
        ImGui::MenuItem("Inspectors", nullptr, true, false);
        ImGui::MenuItem("Diagnostics", nullptr, true, false);
        ImGui::EndMenu();
    }

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
    ImGui::TextDisabled("|  Iteration 2: desktop windowed IDE shell");

    ImGui::EndMenuBar();
}
