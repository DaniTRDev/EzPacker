#include "EzGuiApp.h"
#include <iostream>

int main(int argc, char** argv) {
    std::cout << "Starting EzPacker Compiler Dashboard..." << std::endl;

    EzGui::EzGuiApp app;
    if (!app.Initialize("EzPacker Developer Dashboard", 1280, 800)) {
        std::cerr << "Failed to initialize the GUI application!" << std::endl;
        return -1;
    }

    // Run the main render loop
    app.Run();

    return 0;
}
