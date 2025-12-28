#include "Views/MainMenuBar.h"

const char *MainMenuBar::getName() { return "MainMenuBar"; }

void MainMenuBar::render()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ImGui::MenuItem("Open");
            ImGui::MenuItem("Close");
            ImGui::Separator();

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}
