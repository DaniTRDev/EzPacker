#ifndef EZPACKER_MAINMENUBAR_H
#define EZPACKER_MAINMENUBAR_H

#include "EzFrontendDebugGUICommon.h"
#include "Views/IView.h"

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
};

#endif // EZPACKER_MAINMENUBAR_H
