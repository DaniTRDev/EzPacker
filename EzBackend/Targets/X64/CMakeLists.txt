set(TARGET_X64_FILES_LIST

        "Windows64ABI.cpp"
)

EzCMK_ConfigureLib(NAME "Target_x64"
        FILE_LIST ${TARGET_X64_FILES_LIST}
        PRECOMPILED_HEADER ""
        LINKED_LIBRARIES "EzTarget;asmjit"
        INCLUDED_DIRS "include")

if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for Target_x64")

    #EzPacker_AddTest(
    #        "T_PromoteTypeLegalizer"
    #        "EzTarget"
    #        ""
    #        "tests/T_PromoteTypeLegalizer.cpp"
    #)
endif ()
