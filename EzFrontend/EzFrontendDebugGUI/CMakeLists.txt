EzCMK_AddVendor(https://github.com/ocornut/imgui
        8a944222469ab23559bc01a563f4e9b516e4e701
        imgui
)

FetchContent_GetProperties(imgui)
if (NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif ()

set(IMGUI_FILE_LIST
        "${imgui_SOURCE_DIR}/imgui.h"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.h"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_win32.h"

        "${imgui_SOURCE_DIR}/imgui.cpp"
        "${imgui_SOURCE_DIR}/imgui_demo.cpp"
        "${imgui_SOURCE_DIR}/imgui_draw.cpp"
        "${imgui_SOURCE_DIR}/imgui_tables.cpp"
        "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp"
)

EzCMK_ConfigureLib(NAME "imgui"
        FILE_LIST ${IMGUI_FILE_LIST}
        PRECOMPILED_HEADER ""
        LINKED_LIBRARIES "d3dcompiler;dwmapi"
        INCLUDED_DIRS "${imgui_SOURCE_DIR};${imgui_SOURCE_DIR}/backends")

set(EZFRONTEND_DEBUG_GUI_FILE_LIST
        "include/EzFrontendDebugGUI.h"
        "include/EzFrontendDebugGUICommon.h"
        "include/GUI.h"
        "include/Frontend/EzFrontendWrapper.h"
        "include/Views/Editor.h"
        "include/Views/IView.h"
        "include/Views/Lexer.h"
        "include/Views/MainMenuBar.h"
        "include/Views/Output.h"

        "src/GUI.cpp"
        "src/main.cpp"
        "src/Frontend/EzFrontendWrapper.cpp"
        "src/Views/Editor.cpp"
        "src/Views/Lexer.cpp"
        "src/Views/MainMenuBar.cpp"
        "src/Views/Output.cpp"
)

EzCMK_AddExecutable(NAME EzFrontendDebugGUI
        FILE_LIST "${EZFRONTEND_DEBUG_GUI_FILE_LIST}"
        PRECOMPILED_HEADER "include/EzFrontendDebugGUICommon.h"
        LINKED_LIBRARIES "EzLexer;EzSemantics;EzMir;EzFrontendCompiler;imgui;d3d11"
        INCLUDED_DIRS "include"
)
