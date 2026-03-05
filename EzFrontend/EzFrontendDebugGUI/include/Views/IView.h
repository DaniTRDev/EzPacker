/**
 * @file IView.h
 * @brief Abstract interface for a renderable ImGui panel in the debug GUI.
 *
 * Every debug view (Editor, Output, Lexer, MainMenuBar, …) implements
 * IView so the GUI singleton can iterate and render them uniformly.
 */
#ifndef EZPACKER_IVIEW_H
#define EZPACKER_IVIEW_H

#include "EzFrontendDebugGUICommon.h"

class IView
{
  public:
    virtual ~IView() = default;

    /**
     * Returns the name of the view.
     * @return const char*
     */
    virtual const char* getName() = 0;
    
    /**
     * Renders the view with ImGui.
     */
    virtual void render() = 0;
};

#endif // EZPACKER_IVIEW_H
