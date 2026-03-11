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
    // We could store scroll positions here if ImGui::InputTextMultiline exposed them easily
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

  private:
    static constexpr size_t c_editorBufferSize = 1024 * 1024;

    bool promptOpenFile();
    bool openFile(const std::string &path);
    bool promptSaveFileAs();
    bool saveCurrentFile();
    bool compileCurrentBuffer();

    void syncEditorBufferFromSource(const std::string &source);
    std::string getEditorText() const;

    void renderToolbar();
    void renderWorkspace();
    void renderProjectPanel();
    void renderEditorPanel();
    void renderInspectorPanel();
    void renderDiagnosticsPanel();

    void renderPipelineStages();
    void renderIncludedFiles();
    void renderTokenizer();
    void renderParser();
    void renderSemantics();
    void renderMir();

    void renderAstNodeTree(AstNode *node, const std::string &label);
    void renderScopeTree(const Scope *scope, const char *label, bool includeParents = true);

    std::string formatSeverity(ErrorSeverity severity) const;
    std::string formatSourceReference(const SourceReference &sourceRef) const;
    std::string formatOperand(const MirOperand &operand) const;

    // Helper to switch active file
    void switchToTab(size_t index);
    void closeTab(size_t index);
    void createNewTab();

    // Helper for "Go to Definition" / Error navigation
    void navigateToSource(const SourceReference &sourceRef);

  private:
    std::shared_ptr<EzFrontendWrapper> m_frontend;
    std::vector<char> m_editorBuffer; // Resizable buffer for current editor content
    std::string m_statusMessage;

    // Multi-file support
    std::vector<OpenedFile> m_openedFiles;
    size_t m_activeFileIndex = 0; // Index into m_openedFiles
    
    // For navigation request
    bool m_scrollToLineRequested = false;
    int m_scrollToLine = 0;
};

#endif // EZPACKER_EDITOR_H
