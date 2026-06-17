set(EZTRIPLE_FILES_LIST
        "include/CallLoweringState.h"
        "include/EzTriple.h"
        "include/EzTripleCommon.h"
        "include/DefaultLegalizerActions/PromoteScalarAction.h"
        "include/Descriptors/ABIDesc.h"
        "include/Descriptors/ArgumentLocationDesc.h"
        "include/Descriptors/CallingConvDesc.h"
        "include/Descriptors/TargetDesc.h"
        "include/Legalizer/LegalizeAction.h"
        "include/Legalizer/MirLegalizer.h"
        "include/Legalizer/MirLegalizerPass.h"

        "src/CallLoweringState.cpp"
        "src/DefaultLegalizerActions/PromoteScalarAction.cpp"
        "src/Descriptors/ArgumentLocation.cpp"
        "src/Legalizer/MirLegalizer.cpp"
        "src/Legalizer/MirLegalizerPass.cpp"
)

set(TRIPLETESTSUITE_FILES_LIST
        "tests/EzTripleTestSuite/EzTripleTestSuite.h"
        "tests/EzTripleTestSuite/PromoteScalarActionVerifier.h"

        "tests/EzTripleTestSuite/PromoteScalarActionVerifier.cpp"
)

EzCMK_ConfigureLib(NAME "EzTriple"
        FILE_LIST ${EZTRIPLE_FILES_LIST}
        PRECOMPILED_HEADER "include/EzTripleCommon.h"
        LINKED_LIBRARIES "EzMir"
        INCLUDED_DIRS "include")

if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzTriple")

    EzCMK_ConfigureLib(NAME "EzTripleTestSuite"
            FILE_LIST ${TRIPLETESTSUITE_FILES_LIST}
            PRECOMPILED_HEADER "tests/EzTripleTestSuite/EzTripleTestSuite.h"
            LINKED_LIBRARIES "EzTriple;EzMirTestSuite"
    )

    EzPacker_AddTest(
            "T_PromoteScalarAction"
            "EzTripleTestSuite"
            ""
            "tests/T_PromoteScalarAction.cpp"
    )
endif ()
