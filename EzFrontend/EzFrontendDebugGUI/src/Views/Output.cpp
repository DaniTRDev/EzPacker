#include "Views/Output.h"

Output::Output(const std::shared_ptr<ErrorCollector> &errorCollector) : m_errorCollector(errorCollector) {}

const char *Output::getName() { return "Output"; }

void Output::render()
{
    ImGui::TextUnformatted("Diagnostics are integrated in the IDE bottom panel.");
    ImGui::TextDisabled("This auxiliary view is kept for compatibility with the existing GUI shell.");
    ImGui::Text("Error collector attached: %s", m_errorCollector ? "yes" : "no");
}
