#include "GUI.h"
#include "Views/Lexer.h"
#include "Views/Editor.h"

int main(int, char **)
{
    auto &gui = Gui::get();
    std::shared_ptr<Logger> logger = EzLogger::createSinkLogger("DEBUG_LOGGER");

    if (!gui.initialize(logger))
    {
        logger->pushLog(LogMessage("Could not initialize GUI"));
        return 1;
    }

    gui.loop();

    if (!gui.uninitialize())
    {
        logger->pushLog(LogMessage("Could not uninitialize GUI"));
        return 1;
    }

    return 0;
}