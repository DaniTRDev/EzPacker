#include "Views/Editor.h"

Editor::Editor(std::shared_ptr<EzFrontendWrapper> frontend) :
    m_error(false), m_isFileOpened(false), m_isFileParsed(false), m_openedFilePath(""), m_frontend(std::move(frontend))
{
}

const char *Editor::getName() { return "Editor"; }

void Editor::render()
{
    if (!m_isFileOpened)
    {

        ImGui::Text("No file opened, you can open one");
        ImGui::SameLine();
        if (ImGui::TextLink("here"))
        {
            m_error = false;
            m_fileContent = "";
            m_openedFilePath = "";

            OPENFILENAME ofn;      // common dialog box structure
            char szFile[MAX_PATH]; // buffer for file name
            ZeroMemory(&ofn, sizeof(ofn));
            ZeroMemory(szFile, sizeof(szFile));

            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = nullptr; // no owner window
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = ".ezc\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileName(&ofn))
            {
                m_openedFilePath = std::filesystem::path(szFile);
                m_frontend->getErrorCollector()->information(LogMessage("Opening {} in the editor", szFile));

                std::unique_ptr<uint8_t[]> data;
                if (m_frontend->openAndTokenize(m_openedFilePath, m_fileContentSize, data))
                {
                    m_frontend->getErrorCollector()->information(LogMessage("Opened {}", szFile));

                    m_fileContent = std::string((char *)data.get(), m_fileContentSize);
                    m_isFileOpened = true;
                }
                else
                {
                    m_frontend->getErrorCollector()->information(LogMessage("Could not open or tokenize file"));
                    m_error = true;
                }
            }
            else
            {
                m_frontend->getErrorCollector()->information(
                        LogMessage("ERROR - Failed to open {} in the editor", szFile));
                m_error = true;
            }
        }
    }
    else
    {
        if (ImGui::BeginTabBar(""))
        {
            renderFileContent();
            renderTokenizer();
            renderParser();

            ImGui::EndTabBar();
        }
    }
}

void Editor::renderFileContent()
{
    if (ImGui::BeginTabItem("Editor"))
    {
        const char *ptr = m_fileContent.c_str();
        ImGui::TextUnformatted(ptr, ptr + m_fileContent.size());
        ImGui::EndTabItem();
    }
}

void Editor::renderTokenizer()
{
    if (ImGui::BeginTabItem("Tokens"))
    {
        auto &tokens = m_frontend->getTokens();
        for (size_t i = 0; i < tokens.size(); i++)
        {
            auto &token = tokens[i];
            if (ImGui::TreeNode(std::to_string(i).c_str()))
            {
                ImGui::Text(std::format("Content: {}", token.m_str).c_str());
                ImGui::TreePop();
            }
        }
        ImGui::EndTabItem();
    }
}

void Editor::renderParser()
{
    if (ImGui::BeginTabItem("Parser"))
    {
        if (!m_isFileParsed && !m_error)
        {
            ImGui::Text("Parsing file...");

            if (m_frontend->parse())
            {
                m_frontend->getErrorCollector()->information(LogMessage("Parsed file"));
                m_isFileParsed = true;
            }
            else
            {
                m_frontend->getErrorCollector()->information(LogMessage("Error when parsing file"));
                m_error = true;
            }
        }
        else
        {
            auto &result = m_frontend->getParseResult();

            for (size_t i = 0; i < result.size(); i++)
            {
                auto &expr = result[i];
                std::string label = std::format("{} = {}", i, expr->getAstNodeName());

                if (ImGui::TreeNode(label.c_str()))
                {
                    std::string content = expr->getAsStr(AstNodeStringMode::Debug);
                    ImGui::Text(content.data());
                    ImGui::TreePop();
                }
            }
        }
        ImGui::EndTabItem();
    }
}