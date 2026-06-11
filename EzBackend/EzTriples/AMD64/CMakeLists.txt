set(EZAMD64_TRIPLE_FILES_LIST
        "include/Amd64.h"
        "include/Amd64Legalizer.h"

        "src/Amd64Legalizer.cpp"
)

EzCMK_ConfigureLib(NAME "EzTriple_Amd64"
        FILE_LIST ${EZAMD64_TRIPLE_FILES_LIST}
        PRECOMPILED_HEADER "include/Amd64.h"
        LINKED_LIBRARIES "EzTriple"
        INCLUDED_DIRS "include")

if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzTriple_Amd64")

    #EzPacker_AddTest(
    #        "T_LivenessAnalysis"
    #        "EzMirTestSuite"
    #        ""
    #        "tests/T_LivenessAnalysis.cpp"
    #)
endif ()