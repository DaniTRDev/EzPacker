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
        "${imgui_SOURCE_DIR}/imgui.cpp"
        "${imgui_SOURCE_DIR}/imgui_demo.cpp"
        "${imgui_SOURCE_DIR}/imgui_draw.cpp"
        "${imgui_SOURCE_DIR}/imgui_tables.cpp"
        "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
)

set(IMGUI_LINKED_LIBRARIES "")

if (WIN32)
    list(APPEND IMGUI_FILE_LIST
            "${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.h"
            "${imgui_SOURCE_DIR}/backends/imgui_impl_win32.h"
            "${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp"
            "${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp"
    )
    list(APPEND IMGUI_LINKED_LIBRARIES "d3dcompiler;dwmapi")
elseif (UNIX AND NOT APPLE)
    find_package(glfw3 REQUIRED)
    find_package(OpenGL REQUIRED)

    list(APPEND IMGUI_FILE_LIST
            "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.h"
            "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.h"
            "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
            "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
    )
    list(APPEND IMGUI_LINKED_LIBRARIES "glfw;OpenGL::GL")
endif()

EzCMK_ConfigureLib(NAME "imgui"
        FILE_LIST ${IMGUI_FILE_LIST}
        PRECOMPILED_HEADER ""
        LINKED_LIBRARIES ${IMGUI_LINKED_LIBRARIES}
        INCLUDED_DIRS "${imgui_SOURCE_DIR};${imgui_SOURCE_DIR}/backends")

set(EZFRONTEND_DEBUG_GUI_FILE_LIST
        "include/EzFrontendDebugGUI.h"
        "include/EzFrontendDebugGUICommon.h"
        "include/GUI.h"
        "include/Frontend/EzFrontendWrapper.h"
        "include/Views/Editor.h"
        "include/Views/FileExplorer.h"
        "include/Views/IView.h"
        "include/Views/Lexer.h"
        "include/Views/MainMenuBar.h"
        "include/Views/Output.h"

        "src/GUI.cpp"
        "src/main.cpp"
        "src/Frontend/EzFrontendWrapper.cpp"
        "src/Views/Editor.cpp"
        "src/Views/FileExplorer.cpp"
        "src/Views/Lexer.cpp"
        "src/Views/MainMenuBar.cpp"
        "src/Views/Output.cpp"
)

set(EZFRONTEND_DEBUG_GUI_LIBS "EzLexer;EzSemantics;EzMir;EzFrontendCompiler;imgui")
if(WIN32)
    list(APPEND EZFRONTEND_DEBUG_GUI_LIBS "d3d11")
endif()

EzCMK_AddExecutable(NAME EzFrontendDebugGUI
        FILE_LIST "${EZFRONTEND_DEBUG_GUI_FILE_LIST}"
        PRECOMPILED_HEADER "include/EzFrontendDebugGUICommon.h"
        LINKED_LIBRARIES "${EZFRONTEND_DEBUG_GUI_LIBS}"
        INCLUDED_DIRS "include"
)

# Copy assets folder to the build directory next to the executable
add_custom_command(TARGET EzFrontendDebugGUI POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:EzFrontendDebugGUI>/assets"
    COMMENT "Copying assets directory to build output..."
)
