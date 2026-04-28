set(EZMIRLEGALIZER_FILES_LIST
        "include/EzMirLegalizerCommon.h"
        "include/MirLegalizerContext.h"
        "include/MirTypeLegalizer/MirTypeLegalizerPass.h"

        "src/MirLegalizerContext.cpp"
        "src/MirTypeLegalizer/MirTypeLegalizerPass.cpp"
)

EzCMK_ConfigureLib(NAME "EzMirLegalizer"
        FILE_LIST ${EZMIRLEGALIZER_FILES_LIST}
        PRECOMPILED_HEADER "include/EzMirLegalizerCommon.h"
        LINKED_LIBRARIES "EzMir;EzABI"
        INCLUDED_DIRS "include")

if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzMirLegalizer")

    EzPacker_AddTest(
            "T_MirTypeLegalizer"
            "EzMirLegalizer;EzABI"
            ""
            "tests/T_MirTypeLegalizer.cpp"
    )

    EzPacker_AddTest(
            "T_MirLegalizerContext"
            "EzMirLegalizer;EzABI"
            ""
            "tests/T_MirLegalizerContext.cpp"
    )
endif ()
