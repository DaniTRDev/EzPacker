#include "Views/FileExplorer.h"
#include "Views/Editor.h"

FileExplorer::FileExplorer(std::shared_ptr<Editor> editor)
    : m_editor(std::move(editor)), m_rootPath(std::filesystem::current_path())
{
    m_rootPath = std::filesystem::absolute(m_rootPath);
    const std::string rootStr = m_rootPath.string();
    strncpy(m_browsePathBuffer, rootStr.c_str(), sizeof(m_browsePathBuffer) - 1);
}

const char *FileExplorer::getName() { return "File Explorer"; }

void FileExplorer::setRootPath(const std::filesystem::path &path)
{
    if (std::filesystem::exists(path) && std::filesystem::is_directory(path))
    {
        m_rootPath = std::filesystem::absolute(path);
        const std::string rootStr = m_rootPath.string();
        strncpy(m_browsePathBuffer, rootStr.c_str(), sizeof(m_browsePathBuffer) - 1);
    }
}

std::filesystem::path FileExplorer::getTargetDirectory() const
{
    if (!m_selectedPath.empty())
    {
        if (std::filesystem::is_directory(m_selectedPath))
            return m_selectedPath;
        if (m_selectedPath.has_parent_path())
            return m_selectedPath.parent_path();
    }
    return m_rootPath;
}

// ═══════════════════════════════════════════════════════════════════════════
void FileExplorer::render()
{
    renderToolbar();
    ImGui::Separator();

    // ── Tree ─────────────────────────────────────────────────────────────
    if (ImGui::BeginChild("##fe-tree", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
    {
        renderDirectoryNode(m_rootPath);
    }
    ImGui::EndChild();

    // Dialogs must run every frame at this scope level
    renderDialogs();
}

// ═══════════════════════════════════════════════════════════════════════════
void FileExplorer::renderToolbar()
{
    // Toolbar row: actions for the selected directory
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, 1));

    if (ImGui::SmallButton("+File"))
        requestNewFile(getTargetDirectory());
    ImGui::SameLine();

    if (ImGui::SmallButton("+Dir"))
        requestNewDirectory(getTargetDirectory());
    ImGui::SameLine();

    if (ImGui::SmallButton("^"))
    {
        if (m_rootPath.has_parent_path() && m_rootPath.has_relative_path())
            m_rootPath = m_rootPath.parent_path();
    }
    ImGui::SameLine();

    if (ImGui::SmallButton("..."))
        m_showBrowseDialog = true;

    ImGui::PopStyleVar();

    // Root label
    ImGui::TextDisabled("%s", m_rootPath.filename().string().c_str());
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", m_rootPath.string().c_str());
}

// ═══════════════════════════════════════════════════════════════════════════
void FileExplorer::renderDirectoryNode(const std::filesystem::path &path)
{
    try
    {
        std::vector<std::filesystem::directory_entry> directories;
        std::vector<std::filesystem::directory_entry> files;

        for (const auto &entry : std::filesystem::directory_iterator(path))
        {
            if (entry.is_directory())
                directories.push_back(entry);
            else
                files.push_back(entry);
        }

        // ── Directories ──────────────────────────────────────────────────
        for (const auto &entry : directories)
        {
            const std::string filename = entry.path().filename().string();
            if (filename.empty() || filename[0] == '.') continue;

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
                                     | ImGuiTreeNodeFlags_OpenOnDoubleClick
                                     | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (m_selectedPath == entry.path())
                flags |= ImGuiTreeNodeFlags_Selected;

            const bool isOpen = ImGui::TreeNodeEx(filename.c_str(), flags);

            if (ImGui::IsItemClicked())
                m_selectedPath = entry.path();

            if (ImGui::BeginPopupContextItem())
            {
                handleContextMenu(entry.path(), true);
                ImGui::EndPopup();
            }

            if (isOpen)
            {
                renderDirectoryNode(entry.path());
                ImGui::TreePop();
            }
        }

        // ── Files ────────────────────────────────────────────────────────
        for (const auto &entry : files)
        {
            const std::string filename = entry.path().filename().string();
            if (filename.empty() || filename[0] == '.') continue;

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf
                                     | ImGuiTreeNodeFlags_NoTreePushOnOpen
                                     | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (m_selectedPath == entry.path())
                flags |= ImGuiTreeNodeFlags_Selected;

            ImGui::TreeNodeEx(filename.c_str(), flags);

            if (ImGui::IsItemClicked())
            {
                m_selectedPath = entry.path();
                if (ImGui::IsMouseDoubleClicked(0) && m_editor)
                    m_editor->openFile(std::filesystem::absolute(entry.path()).string());
            }

            if (ImGui::BeginPopupContextItem())
            {
                handleContextMenu(entry.path(), false);
                ImGui::EndPopup();
            }
        }
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %s", e.what());
    }
}

// ═══════════════════════════════════════════════════════════════════════════
void FileExplorer::handleContextMenu(const std::filesystem::path &path, bool isDirectory)
{
    if (isDirectory)
    {
        if (ImGui::MenuItem("New File Here"))
            requestNewFile(path);
        if (ImGui::MenuItem("New Directory Here"))
            requestNewDirectory(path);
        ImGui::Separator();
    }

    if (ImGui::MenuItem("Rename"))
        requestRename(path);
    if (ImGui::MenuItem("Delete"))
        requestDelete(path);
}

// ═══════════════════════════════════════════════════════════════════════════
void FileExplorer::requestRename(const std::filesystem::path &path)
{
    m_dialogTarget = path;
    const std::string fn = path.filename().string();
    strncpy(m_dialogInputBuffer, fn.c_str(), sizeof(m_dialogInputBuffer) - 1);
    m_dialogInputBuffer[sizeof(m_dialogInputBuffer) - 1] = '\0';
    m_showRenameDialog = true;
}

void FileExplorer::requestDelete(const std::filesystem::path &path)
{
    m_dialogTarget = path;
    m_showDeleteDialog = true;
}

void FileExplorer::requestNewFile(const std::filesystem::path &parent)
{
    m_dialogTarget = parent;
    memset(m_dialogInputBuffer, 0, sizeof(m_dialogInputBuffer));
    m_showNewFileDialog = true;
}

void FileExplorer::requestNewDirectory(const std::filesystem::path &parent)
{
    m_dialogTarget = parent;
    memset(m_dialogInputBuffer, 0, sizeof(m_dialogInputBuffer));
    m_showNewDirDialog = true;
}

// ═══════════════════════════════════════════════════════════════════════════
void FileExplorer::renderDialogs()
{
    // ── Browse / Change Root ─────────────────────────────────────────────
    if (m_showBrowseDialog)
    {
        ImGui::OpenPopup("Change Root##Dialog");
        const std::string rootStr = m_rootPath.string();
        strncpy(m_browsePathBuffer, rootStr.c_str(), sizeof(m_browsePathBuffer) - 1);
        m_showBrowseDialog = false;
    }
    if (ImGui::BeginPopupModal("Change Root##Dialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Enter the new root directory path:");
        const bool enter = ImGui::InputText("##browse_input",
                                             m_browsePathBuffer,
                                             sizeof(m_browsePathBuffer),
                                             ImGuiInputTextFlags_EnterReturnsTrue);
        // Preview validity
        const std::filesystem::path candidate(m_browsePathBuffer);
        const bool valid = std::filesystem::exists(candidate) && std::filesystem::is_directory(candidate);
        if (!valid)
            ImGui::TextColored(ImVec4(0.95f, 0.4f, 0.35f, 1.0f), "Path does not exist or is not a directory.");

        if ((ImGui::Button("OK") || enter) && valid)
        {
            setRootPath(candidate);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // ── Rename ───────────────────────────────────────────────────────────
    if (m_showRenameDialog)
    {
        ImGui::OpenPopup("Rename##Dialog");
        m_showRenameDialog = false;
    }
    if (ImGui::BeginPopupModal("Rename##Dialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Rename '%s':", m_dialogTarget.filename().string().c_str());
        const bool enter = ImGui::InputText("##rename_input", m_dialogInputBuffer, sizeof(m_dialogInputBuffer),
                                             ImGuiInputTextFlags_EnterReturnsTrue);
        if (ImGui::Button("OK") || enter)
        {
            const std::filesystem::path newPath = m_dialogTarget.parent_path() / m_dialogInputBuffer;
            try { std::filesystem::rename(m_dialogTarget, newPath); } catch (...) {}
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // ── Delete ───────────────────────────────────────────────────────────
    if (m_showDeleteDialog)
    {
        ImGui::OpenPopup("Delete##Dialog");
        m_showDeleteDialog = false;
    }
    if (ImGui::BeginPopupModal("Delete##Dialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Delete '%s'?", m_dialogTarget.filename().string().c_str());
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "This cannot be undone.");
        if (ImGui::Button("Delete"))
        {
            try { std::filesystem::remove_all(m_dialogTarget); } catch (...) {}
            if (m_selectedPath == m_dialogTarget) m_selectedPath.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // ── New File ─────────────────────────────────────────────────────────
    if (m_showNewFileDialog)
    {
        ImGui::OpenPopup("New File##Dialog");
        m_showNewFileDialog = false;
    }
    if (ImGui::BeginPopupModal("New File##Dialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Create file in '%s':", m_dialogTarget.filename().string().c_str());
        const bool enter = ImGui::InputText("##newfile_input", m_dialogInputBuffer, sizeof(m_dialogInputBuffer),
                                             ImGuiInputTextFlags_EnterReturnsTrue);
        if (ImGui::Button("Create") || enter)
        {
            const std::filesystem::path newPath = m_dialogTarget / m_dialogInputBuffer;
            { std::ofstream out(newPath); } // create empty file on disk
            if (m_editor)
                m_editor->openFile(std::filesystem::absolute(newPath).string());
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // ── New Directory ────────────────────────────────────────────────────
    if (m_showNewDirDialog)
    {
        ImGui::OpenPopup("New Directory##Dialog");
        m_showNewDirDialog = false;
    }
    if (ImGui::BeginPopupModal("New Directory##Dialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Create directory in '%s':", m_dialogTarget.filename().string().c_str());
        const bool enter = ImGui::InputText("##newdir_input", m_dialogInputBuffer, sizeof(m_dialogInputBuffer),
                                             ImGuiInputTextFlags_EnterReturnsTrue);
        if (ImGui::Button("Create") || enter)
        {
            const std::filesystem::path newPath = m_dialogTarget / m_dialogInputBuffer;
            try { std::filesystem::create_directory(newPath); } catch (...) {}
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}
