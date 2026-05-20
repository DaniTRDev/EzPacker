set(EZTARGETEMITTER_FILES_LIST
        "include/CodeBuffer.h"
        "include/CodeSection.h"
        "include/EzTargetEmitter.h"
        "include/EzTargetEmitterCommon.h"

        "src/CodeBuffer.cpp"
        "src/CodeSection.cpp"
)

EzCMK_ConfigureLib(NAME "EzTargetEmitter"
        FILE_LIST ${EZTARGETEMITTER_FILES_LIST}
        PRECOMPILED_HEADER "include/EzTargetEmitterCommon.h"
        LINKED_LIBRARIES "EzTarget"
        INCLUDED_DIRS "include")

if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzTargetEmitter")

    #EzPacker_AddTest(
    #        "T_PromoteTypeLegalizer"
    #        "EzTarget"
    #        ""
    #        "tests/T_PromoteTypeLegalizer.cpp"
    #)

endif ()
