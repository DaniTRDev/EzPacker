set(EZTRIPLE_FILES_LIST
        "include/CallLoweringState.h"
        "include/EzTriple.h"
        "include/EzTripleCommon.h"
        "include/Descriptors/ABIDesc.h"
        "include/Descriptors/ArgumentLocationDesc.h"
        "include/Descriptors/CallingConvDesc.h"

        "src/CallLoweringState.cpp"
        "src/Descriptors/ArgumentLocation.cpp"
)

EzCMK_ConfigureLib(NAME "EzTriple"
        FILE_LIST ${EZTRIPLE_FILES_LIST}
        PRECOMPILED_HEADER "include/EzTripleCommon.h"
        LINKED_LIBRARIES "EzMir"
        INCLUDED_DIRS "include")

if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzTriple")

    #EzPacker_AddTest(
    #        "T_LivenessAnalysis"
    #        "EzMirTestSuite"
    #        ""
    #        "tests/T_LivenessAnalysis.cpp"
    #)
endif ()
