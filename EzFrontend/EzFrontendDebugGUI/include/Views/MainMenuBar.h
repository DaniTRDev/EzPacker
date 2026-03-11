/**
 * @file MainMenuBar.h
 * @brief ImGui view that renders the top-level menu bar (File, Edit, …).
 */
#ifndef EZPACKER_MAINMENUBAR_H
#define EZPACKER_MAINMENUBAR_H

#include "EzFrontendDebugGUICommon.h"
#include "Views/IView.h"

class Editor;      // forward declaration
class FileExplorer; // forward declaration

class MainMenuBar : public IView
{
  public:
    /**
     * Returns the name of the view.
     * @return const char*
     */
    const char* getName() override;
    
    /**
     * Renders the main menu bar with options such as File, Edit, ...
     */
    void render() override;

    /** Wire up the editor instance so menu actions can delegate to it. */
    void setEditor(std::shared_ptr<Editor> editor)           { m_editor = std::move(editor); }

    /** Wire up the file explorer so View menu can toggle its visibility. */
    void setFileExplorer(std::shared_ptr<FileExplorer> fe)   { m_fileExplorer = std::move(fe); }

  private:
    std::shared_ptr<Editor>       m_editor;
    std::shared_ptr<FileExplorer> m_fileExplorer;

    bool m_showFileExplorer = true;
};

#endif // EZPACKER_MAINMENUBAR_H
