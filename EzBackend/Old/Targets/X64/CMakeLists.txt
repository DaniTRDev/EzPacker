set(TARGET_X64_FILES_LIST
        "x64Emitter.h"
        "x64InstrSelector.h"
        "x64TypeLegalizer.h"

        "x64Emitter.cpp"
        "x64InstrSelector.cpp"
        "x64TypeLegalizer.cpp"
)

EzCMK_ConfigureLib(NAME "Target_x64"
        FILE_LIST ${TARGET_X64_FILES_LIST}
        PRECOMPILED_HEADER "x64TypeLegalizer.h"
        LINKED_LIBRARIES "EzTargetEmitter;asmjit"
        INCLUDED_DIRS "include")

if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for Target_x64")
    add_subdirectory(tests)
endif ()
