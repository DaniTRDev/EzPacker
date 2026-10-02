set(EZLINKER_LIB_FILES_LIST
        "include/EzLinkerCommon.h"
        "include/SystemLinkerDetector.h"
        "include/EzLinkerDriver.h"

        "src/SystemLinkerDetector.cpp"
        "src/EzLinkerDriver.cpp"
)

EzCMK_ConfigureLib(NAME "EzLinkerLib"
        FILE_LIST ${EZLINKER_LIB_FILES_LIST}
        PRECOMPILED_HEADER "include/EzLinkerCommon.h"
        LINKED_LIBRARIES "EzCore;argparse"
        INCLUDED_DIRS "include;../EzCore/include"
)

# Exception Runtime Helper Library (static library for SjLj exceptions)
add_library(EzExceptionRuntime STATIC
        "runtime/EzExceptionRuntime.h"
        "runtime/EzExceptionRuntime.cpp"
)
target_include_directories(EzExceptionRuntime PUBLIC
        "runtime"
        "${CMAKE_CURRENT_SOURCE_DIR}/../EzCore/include"
)

EzCMK_AddExecutable(NAME "ez-ld"
        FILE_LIST "src/Main.cpp"
        PRECOMPILED_HEADER "include/EzLinkerCommon.h"
        LINKED_LIBRARIES "EzLinkerLib;EzExceptionRuntime;argparse"
        INCLUDED_DIRS "include;runtime;../EzCore/include"
)

# Alias target so 'cmake --build . --target EzLinker' works seamlessly
add_custom_target(EzLinker DEPENDS ez-ld)
