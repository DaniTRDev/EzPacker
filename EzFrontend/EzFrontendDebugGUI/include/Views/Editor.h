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

  private:
    std::shared_ptr<EzFrontendWrapper> m_frontend;
    std::array<char, c_editorBufferSize> m_editorBuffer{};
    std::string m_statusMessage;
    bool m_dirty{ false };
};

#endif // EZPACKER_EDITOR_H
