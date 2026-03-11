/**
 * @file FileExplorer.h
 * @brief View for browsing the project workspace and opening files.
 */
#ifndef EZPACKER_FILEEXPLORER_H
#define EZPACKER_FILEEXPLORER_H

#include "Views/IView.h"
#include <filesystem>
#include <vector>
#include <functional>

class Editor; // Forward declaration

class FileExplorer : public IView
{
  public:
    explicit FileExplorer(std::shared_ptr<Editor> editor);

    const char *getName() override;
    void render() override;

    /** Programmatically change the root directory. */
    void setRootPath(const std::filesystem::path &path);

  private:
    void renderToolbar();
    void renderDirectoryNode(const std::filesystem::path &path);
    void handleContextMenu(const std::filesystem::path &path, bool isDirectory);

    // Returns the best "target directory" for new-file / new-dir actions:
    // if the selected path is a directory use it, otherwise use its parent,
    // falling back to m_rootPath.
    std::filesystem::path getTargetDirectory() const;

    // File operations
    void requestRename(const std::filesystem::path &path);
    void requestDelete(const std::filesystem::path &path);
    void requestNewFile(const std::filesystem::path &parent);
    void requestNewDirectory(const std::filesystem::path &parent);

    // Dialog helpers
    void renderDialogs();

  private:
    std::shared_ptr<Editor> m_editor;
    std::filesystem::path m_rootPath;
    std::filesystem::path m_selectedPath;
    std::filesystem::path m_dialogTarget;

    // Dialog state
    bool m_showRenameDialog   = false;
    bool m_showDeleteDialog   = false;
    bool m_showNewFileDialog  = false;
    bool m_showNewDirDialog   = false;
    bool m_showBrowseDialog   = false;

    char m_dialogInputBuffer[256]  = {};
    char m_browsePathBuffer[512]   = {};
};

#endif // EZPACKER_FILEEXPLORER_H
