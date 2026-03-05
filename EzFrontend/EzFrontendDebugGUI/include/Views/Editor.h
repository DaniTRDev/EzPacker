/**
 * @file Editor.h
 * @brief ImGui view that provides a simple code editor with tokenize/parse
 *        buttons and displays the resulting AST.
 */
#ifndef EZPACKER_EDITOR_H
#define EZPACKER_EDITOR_H

#include "EzFrontendDebugGUICommon.h"
#include "Views/IView.h"
#include "Frontend/EzFrontendWrapper.h"
#include "Views/Output.h"

class Editor : public IView
{
  public:
    /**
     * Creates the frontend editor linked to given frontend instance and logging sink.
     * @param frontend
     */
    Editor(std::shared_ptr<EzFrontendWrapper> frontend);

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
    /**
     * Renders any error as a sub-window.
     */
    void renderErrorWindow();

    /**
     * Renders the content of the file as a sub-window.
     */
    void renderFileContent();

    /**
     * Renders the result of the tokenizer as a sub-window.
     */
    void renderTokenizer();

    /**
     * Renders the result of the parser as a sub-window.
     */
    void renderParser();

  private:
    bool m_error;
    bool m_isFileOpened;
    bool m_isFileParsed;
    size_t m_fileContentSize;
    std::filesystem::path m_openedFilePath;
    std::shared_ptr<EzFrontendWrapper> m_frontend;
    std::string m_fileContent;
};

#endif // EZPACKER_EDITOR_H
