#include "Views/Output.h"

Output::Output(const std::shared_ptr<ErrorCollector> &errorCollector) : m_errorCollector(errorCollector)
{
    errorCollector->addPipe(
            { .m_onErrorCallback =
                      [this](LogMessage msg)
              {
                  m_errors.push_back(std::move(msg));
              },

              .m_onInfoCallback =
                      [this](LogMessage msg)
              {
                  m_info.push_back(std::move(msg));
              } });
}

const char *Output::getName() { return "Output"; }

void Output::render()
{
    if (ImGui::BeginTabBar("Output"))
    {
        if (ImGui::BeginTabItem("Errors"))
        {
            for (auto &msg : m_errors)
            {
                ImGui::Text(msg.getRawMessage().c_str());
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Information"))
        {
            for (auto &msg : m_info)
            {
                ImGui::Text(msg.getRawMessage().c_str());
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}
