/**
 * @file Editor.h
 * @brief ImGui view that provides a simple code editor with tokenize/parse
 *        buttons and displays the resulting AST.
 */
#ifndef EZPACKER_EDITOR_H
#define EZPACKER_EDITOR_H

#include "EzFrontendDebugGUICommon.h"
#include "Frontend/EzFrontendWrapper.h"
#include "Views/IView.h"

struct OpenedFile
{
    std::string m_path;
    std::string m_content;
    bool m_dirty = false;
    std::filesystem::file_time_type m_lastWriteTime{};
};

class Editor : public IView
{
  public:
    /**
     * Creates the frontend editor linked to given frontend instance and logging sink.
     * @param frontend
     */
    explicit Editor(std::shared_ptr<EzFrontendWrapper> frontend);

    /**
     * Returns "Editor".
     * @return const char*
     */
    const char *getName() override;

    /**
     * Renders the editor window.
     */
    void render() override;

    /**
     * Opens a file by absolute path in a new tab (or focuses the existing tab).
     * Called by FileExplorer when the user double-clicks a file.
     * @param path Absolute path to the file on disk.
     * @return true if the file was opened successfully.
     */
    bool openFile(const std::string &path);

    /** Convenience alias forwarding to openFile(). */
    bool openFileFromExplorer(const std::string &path) { return openFile(path); }

    // ── Toolbar actions — also callable from MainMenuBar ─────────────────
    bool promptOpenFile();
    bool promptSaveFileAs();
    bool saveCurrentFile();
    bool compileCurrentBuffer();
    void createNewTab(); // public — called by MainMenuBar and FileExplorer

  private:
    void syncEditorBufferFromSource(const std::string &source);
    [[nodiscard]] std::string getEditorText() const;

    void renderToolbar();
    void renderWorkspace();
    void renderEditorPanel();
    void renderInspectorPanel();
    void renderDiagnosticsPanel();

    void renderTokenizer();
    void renderParser();
    void renderSemantics();
    void renderMir();

    void renderAstNodeTree(AstNode *node, const std::string &label);
    void renderScopeTree(const Scope *scope, const char *label, bool includeParents = true);

    [[nodiscard]] static std::string formatSeverity(ErrorSeverity severity);
    [[nodiscard]] std::string formatSourceReference(const SourceReference &sourceRef) const;
    [[nodiscard]] static std::string formatOperand(const MirOperand &operand);

    void switchToTab(size_t index);
    void closeTab(size_t index);
    void navigateToSource(const SourceReference &sourceRef);
    void checkExternalFileModifications();

  private:
    static constexpr size_t c_editorBufferSize = 1024 * 1024;

    std::shared_ptr<EzFrontendWrapper> m_frontend;
    std::vector<char> m_editorBuffer; // Resizable buffer for current editor content
    std::string m_statusMessage;

    // Multi-file support
    std::vector<OpenedFile> m_openedFiles;
    size_t m_activeFileIndex = 0; // Index into m_openedFiles
    
    // For navigation request
    bool m_scrollToLineRequested = false;
    int  m_scrollToLine = 0;

    // Set to true by switchToTab() so renderEditorPanel applies SetSelected once
    bool m_pendingTabSwitch = false;

    // Throttle external-modification checks (frame counter)
    int m_fileCheckFrameCounter = 0;
    static constexpr int c_fileCheckInterval = 60; // check every N frames

    // When true, the next InputTextMultiline render will reset cursor to position 0
    bool m_resetCursorRequested = false;
};

#endif // EZPACKER_EDITOR_H
