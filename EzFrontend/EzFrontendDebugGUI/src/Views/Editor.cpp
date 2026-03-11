#include "Views/Editor.h"
#include <imgui_internal.h>

Editor::Editor(std::shared_ptr<EzFrontendWrapper> frontend) : m_frontend(std::move(frontend))
{
    m_editorBuffer.resize(c_editorBufferSize);
    std::fill(m_editorBuffer.begin(), m_editorBuffer.end(), '\0');
    m_statusMessage = "Ready. Open an .ez file or create a new scratch buffer.";
}

const char *Editor::getName() { return "Editor"; }

void Editor::render()
{
    renderToolbar();
    ImGui::Separator();
    renderWorkspace();
}

bool Editor::promptOpenFile()
{
#ifdef _WIN32
    OPENFILENAMEA ofn{};
    char fileName[MAX_PATH] = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "EZ source (*.ez)\0*.ez\0All files (*.*)\0*.*\0\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (!GetOpenFileNameA(&ofn))
    {
        return false;
    }

    return openFile(fileName);
#else
    m_statusMessage = "File dialogs not supported on this platform yet.";
    return false;
#endif
}

bool Editor::openFile(const std::string &path)
{
    // Check if file is already open
    for (size_t i = 0; i < m_openedFiles.size(); ++i)
    {
        // Simple path comparison
        if (m_openedFiles[i].m_path == path)
        {
            switchToTab(i);
            return true;
        }
    }

    if (!m_frontend->loadSourceFromFile(path))
    {
        m_statusMessage = std::format("Could not open '{}'", path);
        return false;
    }

    // New file, create tab
    OpenedFile file;
    file.m_path = path;
    file.m_content = m_frontend->getLoadedSourceText();
    file.m_dirty = false;

    // Record the file's last-write time for external modification detection
    try
    {
        file.m_lastWriteTime = std::filesystem::last_write_time(path);
    }
    catch (const std::filesystem::filesystem_error &)
    {
    }

    m_openedFiles.push_back(file);
    switchToTab(m_openedFiles.size() - 1);

    m_statusMessage = std::format("Opened '{}'", path);
    compileCurrentBuffer();
    return true;
}

bool Editor::promptSaveFileAs()
{
    if (m_openedFiles.empty())
        return false;
#ifdef _WIN32
    OPENFILENAMEA ofn{};
    char fileName[MAX_PATH] = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = "ez";
    ofn.lpstrFilter = "EZ source (*.ez)\0*.ez\0All files (*.*)\0*.*\0\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    if (!GetSaveFileNameA(&ofn))
    {
        return false;
    }

    if (!m_frontend->saveSourceToFile(fileName, getEditorText()))
    {
        m_statusMessage = std::format("Could not save '{}'", fileName);
        return false;
    }

    // Update current tab
    m_openedFiles[m_activeFileIndex].m_path = fileName;
    m_openedFiles[m_activeFileIndex].m_dirty = false;
    m_frontend->loadSourceFromFile(fileName);

    // Refresh last-write time after saving
    try
    {
        m_openedFiles[m_activeFileIndex].m_lastWriteTime = std::filesystem::last_write_time(fileName);
    }
    catch (const std::filesystem::filesystem_error &)
    {
    }

    m_statusMessage = std::format("Saved '{}'", fileName);
    return true;
#else
    m_statusMessage = "File dialogs not supported on this platform yet.";
    return false;
#endif
}

bool Editor::saveCurrentFile()
{
    if (m_openedFiles.empty())
        return false;

    if (m_openedFiles[m_activeFileIndex].m_path.empty())
    {
        return promptSaveFileAs();
    }

    const std::string path = m_openedFiles[m_activeFileIndex].m_path;
    if (!m_frontend->saveSourceToFile(path, getEditorText()))
    {
        m_statusMessage = std::format("Could not save '{}'", path);
        return false;
    }

    m_openedFiles[m_activeFileIndex].m_dirty = false;
    m_frontend->loadSourceFromFile(path);

    // Refresh last-write time after saving
    try
    {
        m_openedFiles[m_activeFileIndex].m_lastWriteTime = std::filesystem::last_write_time(path);
    }
    catch (const std::filesystem::filesystem_error &)
    {
    }

    m_statusMessage = std::format("Saved '{}'", path);
    return true;
}

bool Editor::compileCurrentBuffer()
{
    if (m_openedFiles.empty())
        return false;

    const std::string sourceText = getEditorText();
    // Update content in memory
    m_openedFiles[m_activeFileIndex].m_content = sourceText;

    const std::filesystem::path loadedPath(m_openedFiles[m_activeFileIndex].m_path);
    const std::filesystem::path workingDirectory =
            loadedPath.empty() ? std::filesystem::current_path() : loadedPath.parent_path();
    const std::string sourceName = loadedPath.empty() ? "scratch.ez" : loadedPath.filename().string();

    const bool ok = m_frontend->compileSource(sourceText, sourceName, workingDirectory);
    m_statusMessage = ok ? std::format("Compilation succeeded for '{}'", sourceName)
                         : std::format("Compilation failed for '{}'", sourceName);
    return ok;
}

void Editor::syncEditorBufferFromSource(const std::string &source)
{
    std::fill(m_editorBuffer.begin(), m_editorBuffer.end(), '\0');
    const size_t copySize = std::min(source.size(), m_editorBuffer.size() - 1);
    std::copy_n(source.data(), copySize, m_editorBuffer.data());
}

std::string Editor::getEditorText() const { return { m_editorBuffer.data() }; }

void Editor::createNewTab()
{
    OpenedFile file;
    file.m_path = "";
    file.m_content = "void main() {\n    nop;\n}\n";
    file.m_dirty = true; // virtual file — not on disk
    m_openedFiles.push_back(file);
    switchToTab(m_openedFiles.size() - 1);
}

void Editor::switchToTab(size_t index)
{
    if (index >= m_openedFiles.size())
        return;

    // Save current buffer to memory before switching
    if (m_activeFileIndex < m_openedFiles.size())
        m_openedFiles[m_activeFileIndex].m_content = getEditorText();

    m_activeFileIndex = index;
    m_pendingTabSwitch = true; // tell renderEditorPanel to apply SetSelected once
    syncEditorBufferFromSource(m_openedFiles[index].m_content);

    // Tell the frontend about this file so diagnostics match
    if (!m_openedFiles[index].m_path.empty())
        m_frontend->loadSourceFromFile(m_openedFiles[index].m_path);
}

void Editor::closeTab(size_t index)
{
    if (index >= m_openedFiles.size())
        return;

    m_openedFiles.erase(m_openedFiles.begin() + static_cast<std::ptrdiff_t>(index));

    if (m_openedFiles.empty())
    {
        // No files left — welcome screen will show
        m_activeFileIndex = 0;
        std::fill(m_editorBuffer.begin(), m_editorBuffer.end(), '\0');
        m_statusMessage = "No files open.";
        return;
    }

    // Clamp the active index
    if (m_activeFileIndex >= m_openedFiles.size())
        m_activeFileIndex = m_openedFiles.size() - 1;
    else if (m_activeFileIndex > index)
        m_activeFileIndex--;

    switchToTab(m_activeFileIndex);
}

void Editor::renderToolbar()
{
    const bool hasFiles = !m_openedFiles.empty();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 2));

    if (ImGui::SmallButton("New"))
        createNewTab();
    ImGui::SameLine();
    if (ImGui::SmallButton("Open"))
        promptOpenFile();

    if (hasFiles)
    {
        ImGui::SameLine();
        if (ImGui::SmallButton("Save"))
            saveCurrentFile();
        ImGui::SameLine();
        if (ImGui::SmallButton("Save As"))
            promptSaveFileAs();
        ImGui::SameLine();
        if (ImGui::SmallButton("Compile"))
            compileCurrentBuffer();
        ImGui::SameLine();
        if (ImGui::SmallButton("Reload") && !m_openedFiles[m_activeFileIndex].m_path.empty())
        {
            if (m_frontend->loadSourceFromFile(m_openedFiles[m_activeFileIndex].m_path))
            {
                m_openedFiles[m_activeFileIndex].m_content = m_frontend->getLoadedSourceText();
                m_openedFiles[m_activeFileIndex].m_dirty = false;
                try
                {
                    m_openedFiles[m_activeFileIndex].m_lastWriteTime =
                            std::filesystem::last_write_time(m_openedFiles[m_activeFileIndex].m_path);
                }
                catch (const std::filesystem::filesystem_error &)
                {
                }
                syncEditorBufferFromSource(m_openedFiles[m_activeFileIndex].m_content);
                m_resetCursorRequested = true;
                m_statusMessage = std::format("Reloaded '{}'", m_openedFiles[m_activeFileIndex].m_path);
                compileCurrentBuffer();
            }
        }
    }

    ImGui::PopStyleVar();

    // ── Status line ───────────────────────────────────────────────────────
    if (hasFiles)
    {
        const bool buildOk = m_frontend->lastCompilationSucceeded();
        const std::string activeFile = m_openedFiles[m_activeFileIndex].m_path.empty()
                ? std::string("scratch.ez")
                : std::filesystem::path(m_openedFiles[m_activeFileIndex].m_path).filename().string();
        const char *dirtyMark = m_openedFiles[m_activeFileIndex].m_dirty ? " *" : "";

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::TextColored(buildOk ? ImVec4(0.35f, 0.85f, 0.45f, 1.0f) : ImVec4(0.95f, 0.45f, 0.35f, 1.0f),
                           "%s",
                           buildOk ? "OK" : "ERR");
        ImGui::SameLine();
        ImGui::TextDisabled("%s%s — %s", activeFile.c_str(), dirtyMark, m_statusMessage.c_str());
    }
    else
    {
        ImGui::SameLine();
        ImGui::TextDisabled("| No files open");
    }
}

void Editor::renderWorkspace()
{
    // Top: editor code area + inspector side-bar
    // Bottom: diagnostics panel
    // Use a proportional split so it adapts to any window height.
    const float totalHeight = ImGui::GetContentRegionAvail().y;
    const float diagnosticsH = std::max(160.0f, totalHeight * 0.28f);
    const float topH = totalHeight - diagnosticsH - ImGui::GetStyle().ItemSpacing.y - 1.0f;

    if (ImGui::BeginChild("##workspace-top", ImVec2(0, topH), false))
    {
        if (ImGui::BeginTable("##ide-layout-main", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("Editor", ImGuiTableColumnFlags_WidthStretch, 0.0f);
            ImGui::TableSetupColumn("Inspectors", ImGuiTableColumnFlags_WidthFixed, 390.0f);

            ImGui::TableNextColumn();
            renderEditorPanel();

            ImGui::TableNextColumn();
            renderInspectorPanel();

            ImGui::EndTable();
        }
    }
    ImGui::EndChild();

    ImGui::Separator();
    if (ImGui::BeginChild("##workspace-bottom", ImVec2(0, 0), true))
    {
        renderDiagnosticsPanel();
    }
    ImGui::EndChild();
}

void Editor::renderEditorPanel()
{
    // ── Empty state: no files open ───────────────────────────────────────
    if (m_openedFiles.empty())
    {
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        ImGui::Dummy(ImVec2(0, avail.y * 0.3f));

        // Center the text block
        const float textWidth = 340.0f;
        ImGui::SetCursorPosX((avail.x - textWidth) * 0.5f);
        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.56f, 0.74f, 0.96f, 1.0f));
        ImGui::TextUnformatted("EZ Language Frontend IDE");
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::TextDisabled("No files are open.");
        ImGui::TextDisabled("Open a file from the Explorer, or create a scratch buffer.");
        ImGui::Spacing();
        if (ImGui::Button("Create Test File (scratch.ez)", ImVec2(textWidth, 0)))
        {
            createNewTab(); // creates a dirty virtual file
        }
        ImGui::EndGroup();
        return;
    }

    // ── Tab bar ──────────────────────────────────────────────────────────
    if (ImGui::BeginTabBar("##editor-tabs",
                           ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs |
                                   ImGuiTabBarFlags_FittingPolicyScroll))
    {
        for (size_t i = 0; i < m_openedFiles.size(); ++i)
        {
            bool tabOpen = true;
            std::string label = m_openedFiles[i].m_path.empty()
                    ? std::format("scratch.ez##{}", i)
                    : std::format("{}##{}", std::filesystem::path(m_openedFiles[i].m_path).filename().string(), i);

            // Build a display label with dirty marker
            std::string displayLabel = m_openedFiles[i].m_path.empty()
                    ? "scratch.ez"
                    : std::filesystem::path(m_openedFiles[i].m_path).filename().string();
            if (m_openedFiles[i].m_dirty)
                displayLabel += " *";
            // Use the unique label for ImGui IDs but show displayLabel text
            // ImGui uses the part before ## for display and the full string for ID
            std::string tabLabel = displayLabel + std::format("##{}", i);

            // Only force-select a tab once after switchToTab sets the pending flag
            int flags = 0;
            if (m_pendingTabSwitch && i == m_activeFileIndex)
            {
                flags = ImGuiTabItemFlags_SetSelected;
                m_pendingTabSwitch = false;
            }

            if (ImGui::BeginTabItem(tabLabel.c_str(), &tabOpen, flags))
            {
                // If the user clicked this tab, update our active index
                if (i != m_activeFileIndex)
                    switchToTab(i);

                // ── Editor input (unique ID per tab) ─────────────────────
                const ImVec2 size = ImGui::GetContentRegionAvail();
                const std::string inputId = std::format("##ez-source-{}", i);

                if (ImGui::InputTextMultiline(inputId.c_str(),
                                              m_editorBuffer.data(),
                                              m_editorBuffer.size(),
                                              size,
                                              ImGuiInputTextFlags_AllowTabInput))
                {
                    m_openedFiles[m_activeFileIndex].m_dirty = true;
                }

                // Reset cursor to the beginning after a reload
                if (m_resetCursorRequested)
                {
                    if (ImGuiInputTextState *state = ImGui::GetInputTextState(ImGui::GetItemID()))
                    {
                        // Tell ImGui to reload from the user buffer and place
                        // the cursor + selection at position 0.
                        state->ReloadSelectionStart = 0;
                        state->ReloadSelectionEnd = 0;
                        state->WantReloadUserBuf = true;
                        state->Scroll = ImVec2(0.0f, 0.0f);
                    }
                    m_resetCursorRequested = false;
                }

                if (m_scrollToLineRequested)
                {
                    m_statusMessage = std::format("Jump to line {} requested", m_scrollToLine);
                    m_scrollToLineRequested = false;
                }

                ImGui::EndTabItem();
            }

            if (!tabOpen)
            {
                closeTab(i);
                if (i > 0)
                    --i; // re-check index after removal
            }
        }
        ImGui::EndTabBar();
    }
}

void Editor::renderInspectorPanel()
{
    ImGui::TextUnformatted("Inspectors");
    ImGui::Separator();

    if (ImGui::BeginTabBar("##inspectors"))
    {
        renderTokenizer();
        renderParser();
        renderSemantics();
        renderMir();
        ImGui::EndTabBar();
    }
}

void Editor::renderDiagnosticsPanel()
{
    ImGui::TextUnformatted("Diagnostics & Output");
    ImGui::Separator();

    const auto &diagnostics = m_frontend->getDiagnostics();
    if (diagnostics.empty())
    {
        ImGui::TextDisabled(
                "No diagnostics yet. Compile the current buffer to inspect lexer/parser/semantic/MIR output.");
        return;
    }

    if (ImGui::BeginTable(
                "##diagnostics-table",
                5,
                ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Severity", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthFixed, 180.0f);
        ImGui::TableSetupColumn("Sender", ImGuiTableColumnFlags_WidthFixed, 170.0f);
        ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();

        for (const FrontendDiagnosticEntry &diagnostic : diagnostics)
        {
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            const ImVec4 color = diagnostic.m_severity == ErrorSeverity::Fatal
                    ? ImVec4(0.95f, 0.35f, 0.35f, 1.0f)
                    : (diagnostic.m_severity == ErrorSeverity::Warning ? ImVec4(0.95f, 0.8f, 0.3f, 1.0f)
                                                                       : ImVec4(0.6f, 0.75f, 1.0f, 1.0f));
            ImGui::TextColored(color, "%s", formatSeverity(diagnostic.m_severity).c_str());

            ImGui::TableNextColumn();
            // Make source reference clickable
            std::string sourceRefStr = formatSourceReference(diagnostic.m_sourceRef);
            if (ImGui::Selectable(sourceRefStr.c_str(), false, ImGuiSelectableFlags_SpanAllColumns))
            {
                navigateToSource(diagnostic.m_sourceRef);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Click to jump to location");
            }

            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", diagnostic.m_sender.c_str());

            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", diagnostic.m_message.c_str());

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(diagnostic.m_timeStamp.c_str());
        }

        ImGui::EndTable();
    }
}

void Editor::navigateToSource(const SourceReference &sourceRef)
{
    if (!sourceRef.m_valid)
        return;

    std::shared_ptr<SourceManager> sm = m_frontend->getSourceManager();
    if (!sm)
        return;

    std::string filename = std::string(sm->getSourceName(sourceRef.m_sourceFileId));

    // Check if file is already open
    bool found = false;
    for (size_t i = 0; i < m_openedFiles.size(); ++i)
    {
        // Simple check: match end of path or full path
        // In a real app we'd use canonical paths
        std::filesystem::path p1(m_openedFiles[i].m_path);
        std::filesystem::path p2(filename);

        if (m_openedFiles[i].m_path == filename || p1.filename() == p2.filename())
        {
            switchToTab(i);
            found = true;
            break;
        }
    }

    if (!found)
    {
        // Try to open it
        // If filename is relative, might need working dir
        if (openFile(filename))
        {
            // success
        }
        else
        {
            // fallback, maybe it was a scratch buffer or we can't find it
            m_statusMessage = std::format("Could not open file for navigation: {}", filename);
            return;
        }
    }

    // Request scroll
    m_scrollToLineRequested = true;
    m_scrollToLine = static_cast<int>(sourceRef.m_line);
}

void Editor::checkExternalFileModifications()
{
    for (size_t i = 0; i < m_openedFiles.size(); ++i)
    {
        OpenedFile &file = m_openedFiles[i];
        if (file.m_path.empty())
            continue; // scratch buffers have no disk path

        try
        {
            if (!std::filesystem::exists(file.m_path))
                continue;

            const auto currentWriteTime = std::filesystem::last_write_time(file.m_path);
            if (currentWriteTime != file.m_lastWriteTime)
            {
                // File was modified externally — reload it
                if (m_frontend->loadSourceFromFile(file.m_path))
                {
                    file.m_content = m_frontend->getLoadedSourceText();
                    file.m_dirty = false;
                    file.m_lastWriteTime = currentWriteTime;

                    // If this is the active tab, update the editor buffer too
                    if (i == m_activeFileIndex)
                    {
                        syncEditorBufferFromSource(file.m_content);
                        m_resetCursorRequested = true;
                        compileCurrentBuffer();
                    }

                    m_statusMessage = std::format("File '{}' changed on disk — reloaded",
                                                  std::filesystem::path(file.m_path).filename().string());
                }
                else
                {
                    // Update the write time even on failure to avoid retrying every check
                    file.m_lastWriteTime = currentWriteTime;
                }
            }
        }
        catch (const std::filesystem::filesystem_error &)
        {
        }
    }
}

void Editor::renderTokenizer()
{
    if (!ImGui::BeginTabItem("Tokens"))
    {
        return;
    }

    const auto &tokens = m_frontend->getTokens();
    if (tokens.empty())
    {
        ImGui::TextDisabled("No token stream available.");
        ImGui::EndTabItem();
        return;
    }

    if (ImGui::BeginTable(
                "##tokens",
                5,
                ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Text", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Line", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("Col", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < tokens.size(); ++i)
        {
            const TokenInformation &token = tokens[i];
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%zu", i);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(TokenType2StrMap[token.m_type]);
            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", token.m_str.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%zu", token.m_sourceReference.m_line);
            ImGui::TableNextColumn();
            ImGui::Text("%zu", token.m_sourceReference.m_col);
        }

        ImGui::EndTable();
    }

    ImGui::EndTabItem();
}

void Editor::renderParser()
{
    if (!ImGui::BeginTabItem("AST"))
    {
        return;
    }

    const auto &nodes = m_frontend->getParseResult();
    if (nodes.empty())
    {
        ImGui::TextDisabled("No AST available.");
        ImGui::EndTabItem();
        return;
    }

    for (size_t i = 0; i < nodes.size(); ++i)
    {
        renderAstNodeTree(nodes[i], std::format("{}: {}", i, nodes[i]->getAstNodeName()));
    }

    ImGui::EndTabItem();
}

void Editor::renderSemantics()
{
    if (!ImGui::BeginTabItem("Semantics"))
    {
        return;
    }

    const auto &unit = m_frontend->getCompilationUnit();
    if (!unit || !unit->getSemanticContext())
    {
        ImGui::TextDisabled("No semantic context available.");
        ImGui::EndTabItem();
        return;
    }

    auto semanticContext = unit->getSemanticContext();
    ImGui::Text("Global scope available: %s", semanticContext->getGlobalScope() ? "yes" : "no");
    ImGui::Text("Inside loop: %s", semanticContext->isContextInsideLoop() ? "yes" : "no");
    ImGui::Text("Inside switch: %s", semanticContext->isContextInsideSwitch() ? "yes" : "no");
    ImGui::Separator();

    renderScopeTree(semanticContext->getGlobalScope().get(), "Global Scope", false);

    ImGui::EndTabItem();
}

void Editor::renderMir()
{
    if (!ImGui::BeginTabItem("MIR"))
    {
        return;
    }

    const auto &unit = m_frontend->getCompilationUnit();
    if (!unit || !unit->getMirEmitterContext())
    {
        ImGui::TextDisabled("No MIR emitted yet.");
        ImGui::EndTabItem();
        return;
    }

    MirEmitterContext *mirContext = unit->getMirEmitterContext().get();

    //
    // Render Type Table
    //
    if (ImGui::CollapsingHeader("Type Table", ImGuiTreeNodeFlags_DefaultOpen))
    {
        TypedPoolSlice<MirType> *typeList = mirContext->getTypeList();
        if (typeList && typeList->m_numElems > 0)
        {
            if (ImGui::BeginTable("##mir-types",
                                  4,
                                  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
            {
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Kind", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                for (MirType *type : *typeList)
                {
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::Text("%zu", type->getId());

                    ImGui::TableNextColumn();
                    ImGui::Text("%s", type->getName().empty() ? "<anon>" : std::string(type->getName()).c_str());

                    ImGui::TableNextColumn();
                    const char *kindStr = "Unknown";
                    switch (type->getKind())
                    {
                        case MirTypeKind::Invalid:
                            kindStr = "Invalid";
                            break;
                        case MirTypeKind::Integer:
                            kindStr = "Integer";
                            break;
                        case MirTypeKind::FloatingPoint:
                            kindStr = "Float";
                            break;
                        case MirTypeKind::Pointer:
                            kindStr = "Pointer";
                            break;
                        case MirTypeKind::Array:
                            kindStr = "Array";
                            break;
                        case MirTypeKind::Void:
                            kindStr = "Void";
                            break;
                    }
                    ImGui::Text("%s", kindStr);

                    ImGui::TableNextColumn();
                    if (type->getSubTypes() && type->getSubTypes()->m_numElems > 0)
                    {
                        // Just listing subtype IDs for brevity
                        std::string subTypeIds;
                        for (MirType *sub : *type->getSubTypes())
                        {
                            if (!subTypeIds.empty())
                                subTypeIds += ", ";
                            subTypeIds += std::to_string(sub->getId());
                        }
                        ImGui::Text("SubTypes: [%s]", subTypeIds.c_str());
                    }
                    else
                    {
                        ImGui::TextDisabled("-");
                    }
                }
                ImGui::EndTable();
            }
        }
        else
        {
            ImGui::TextDisabled("No types registered.");
        }
    }

    ImGui::Separator();

    //
    // Render Functions
    //
    if (ImGui::CollapsingHeader("Functions", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto *functionList = mirContext->getFunctionList();
        if (!functionList || functionList->m_numElems == 0)
        {
            ImGui::TextDisabled("No MIR functions available.");
        }
        else
        {
            int functionIndex = 0;
            for (MirFunction *function : *functionList)
            {
                MirType *returnType = mirContext->getMirTypeById(function->getReturnTypeId());
                const std::string label =
                        std::format("fn#{} (id={}, return={})",
                                    functionIndex++,
                                    function->getId(),
                                    returnType ? std::string(returnType->getName()) : std::string("<invalid>"));

                if (ImGui::TreeNode(label.c_str()))
                {
                    ImGui::Text("Entry block: %zu",
                                function->getEntryPoint() ? function->getEntryPoint()->getId() : 0ull);
                    ImGui::Text("Parameters: %zu",
                                function->getParameters() ? function->getParameters()->m_numElems : 0ull);

                    if (function->getBlocks())
                    {
                        for (MirBlock *block : *function->getBlocks())
                        {
                            const std::string blockLabel = std::format("block {}", block->getId());
                            if (ImGui::TreeNode(blockLabel.c_str()))
                            {
                                if (block->getInstructions())
                                {
                                    int instructionIndex = 0;
                                    for (MirInstruction *instruction : *block->getInstructions())
                                    {
                                        const MirInstructionMetadata &metadata = instruction->getMetadata();
                                        const std::string instructionLabel =
                                                std::format("{}: {}", instructionIndex++, metadata.m_name);
                                        if (ImGui::TreeNode(instructionLabel.c_str()))
                                        {
                                            ImGui::Text("Opcode: %d", static_cast<int>(instruction->getOpCode()));
                                            ImGui::Text("Flags: 0x%X", instruction->getFlags());
                                            if (instruction->getOperands() &&
                                                instruction->getOperands()->m_numElems > 0)
                                            {
                                                int operandIndex = 0;
                                                for (MirOperand *operand : *instruction->getOperands())
                                                {
                                                    if (operand)
                                                    {
                                                        ImGui::BulletText("op%d = %s",
                                                                          operandIndex++,
                                                                          formatOperand(*operand).c_str());
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                ImGui::TextDisabled("No operands");
                                            }
                                            ImGui::TreePop();
                                        }
                                    }
                                }
                                ImGui::TreePop();
                            }
                        }
                    }
                    ImGui::TreePop();
                }
            }
        }
    }

    ImGui::EndTabItem();
}

void Editor::renderAstNodeTree(AstNode *node, const std::string &label)
{
    if (!node)
    {
        return;
    }

    if (ImGui::TreeNode(label.c_str()))
    {
        ImGui::Text("Node: %s", node->getAstNodeName());
        ImGui::Text("Type: %d", static_cast<int>(node->getType()));
        ImGui::TextWrapped("Source: %s", formatSourceReference(node->getSourceRef()).c_str());
        if (node->hasAnnotations())
        {
            ImGui::Text("Annotations: %zu", node->getAnnotations() ? node->getAnnotations()->m_numElems : 0ull);
        }
        else
        {
            ImGui::TextDisabled("No annotations attached.");
        }
        ImGui::TreePop();
    }
}

void Editor::renderScopeTree(const Scope *scope, const char *label, bool includeParents)
{
    if (!scope)
    {
        ImGui::TextDisabled("No scope available.");
        return;
    }

    if (ImGui::TreeNode(label))
    {
        if (scope->getSymbols().empty())
        {
            ImGui::TextDisabled("No symbols in this scope.");
        }
        else
        {
            for (const auto &[name, symbol] : scope->getSymbols())
            {
                const std::string symbolLabel =
                        std::format("{} [{}]", name, symbol ? symbol->getSymbolTypeName() : "null");
                if (ImGui::TreeNode(symbolLabel.c_str()))
                {
                    if (symbol)
                    {
                        std::string dataType = symbol->getSymbolDataTypeName().empty()
                                ? "NO_TYPE"
                                : std::string(symbol->getSymbolDataTypeName());
                        
                        ImGui::Text("Id: %zu", symbol->getId());
                        ImGui::Text("Kind: %s", symbol->getSymbolTypeName());
                        ImGui::Text("Data type: %s", dataType.c_str());
                        if (symbol->getDefiningNode())
                        {
                            ImGui::Separator();
                            ImGui::Text("Defining node: %s", symbol->getDefiningNode()->getAstNodeName());
                            ImGui::TextWrapped(
                                    "Definition source: %s",
                                    formatSourceReference(symbol->getDefiningNode()->getSourceRef()).c_str());
                        }
                    }
                    ImGui::TreePop();
                }
            }
        }

        if (includeParents && scope->getParent())
        {
            renderScopeTree(scope->getParent(), "Parent Scope", true);
        }
        ImGui::TreePop();
    }
}

std::string Editor::formatSeverity(ErrorSeverity severity)
{
    switch (severity)
    {
        case ErrorSeverity::NoError:
            return "Info";
        case ErrorSeverity::Warning:
            return "Warning";
        case ErrorSeverity::Soft:
            return "Soft";
        case ErrorSeverity::Fatal:
            return "Fatal";
        default:
            return "Unknown";
    }
}

std::string Editor::formatSourceReference(const SourceReference &sourceRef) const
{
    if (!sourceRef.m_valid || !m_frontend->getSourceManager())
    {
        return "<no source>";
    }

    return std::format("{}:{}:{}",
                       std::string(m_frontend->getSourceManager()->getSourceName(sourceRef.m_sourceFileId)),
                       sourceRef.m_line,
                       sourceRef.m_col);
}

std::string Editor::formatOperand(const MirOperand &operand)
{
    switch (operand.getType())
    {
        case MirOperandType::BigInteger:
        {
            const MirBigInteger *value = operand.getBigInteger();
            return std::format("bigint[id={}, size={}]",
                               value ? value->m_constantId : 0ull,
                               value ? value->m_size : 0ull);
        }
        case MirOperandType::Double:
        {
            const MirDouble *value = operand.getDouble();
            return std::format("double({})", value ? value->m_value : 0.0);
        }
        case MirOperandType::Integer:
        {
            const MirInteger *value = operand.getInteger();
            return std::format("int({}, {}B)", value ? value->m_value : 0ll, value ? value->m_size : 0ull);
        }
        case MirOperandType::Memory:
        {
            const MirMemory *value = operand.getMemory();
            return std::format("mem(base=r{}, index=r{}, scale={}, offset={}, size={})",
                               value ? value->m_baseRegId : 0ull,
                               value ? value->m_indexRegId : 0ull,
                               value ? value->m_scale : 0,
                               value ? value->m_offset : 0ll,
                               value ? value->m_size : 0ull);
        }
        case MirOperandType::Reference:
        {
            const MirReference *value = operand.getReference();
            return std::format("ref({})", value ? value->m_refId : 0ull);
        }
        case MirOperandType::Register:
        {
            const MirRegister *value = operand.getRegister();
            return std::format("r{}:{}B", value ? value->m_id : 0ull, value ? value->m_size : 0ull);
        }
        default:
            return "<unknown>";
    }
}
