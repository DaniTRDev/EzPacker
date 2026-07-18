set(EZTRIPLE_FILES_LIST
        "include/EzTriple.h"
        "include/EzTripleCommon.h"
        "include/AbiLowerer/ReturnAbiLowererPass.h"
        "include/DefaultLegalizerActions/ExpandScalarAction.h"
        "include/DefaultLegalizerActions/LegalizeCallAction.h"
        "include/DefaultLegalizerActions/LegalizeReturnAction.h"
        "include/DefaultLegalizerActions/PromoteScalarAction.h"
        "include/Descriptors/ABIDesc.h"
        "include/Descriptors/TargetDesc.h"
        "include/ExpansionRecipe/ExpansionRecipe.h"
        "include/Legalizer/LegalizeAction.h"
        "include/Legalizer/MirBlockLegalizerPass.h"
        "include/Legalizer/MirFunctionSignatureLegalizerPass.h"
        "include/Legalizer/MirLegalizer.h"

        "src/AbiLowerer/ReturnAbiLowererPass.cpp"
        "src/DefaultLegalizerActions/ExpandScalarAction.cpp"
        "src/DefaultLegalizerActions/LegalizeCallAction.cpp"
        "src/DefaultLegalizerActions/LegalizeReturnAction.cpp"
        "src/DefaultLegalizerActions/PromoteScalarAction.cpp"
        "src/Legalizer/MirBlockLegalizerPass.cpp"
        "src/Legalizer/MirFunctionSignatureLegalizerPass.cpp"
        "src/Legalizer/MirLegalizer.cpp"
)

set(TRIPLETESTSUITE_FILES_LIST
        "tests/EzTripleTestSuite/ExpandScalarActionVerifier.h"
        "tests/EzTripleTestSuite/EzTripleTestExpansionRecipe.h"
        "tests/EzTripleTestSuite/EzTripleTestSuite.h"
        "tests/EzTripleTestSuite/EzTripleTestTargetDescriptor.h"
        "tests/EzTripleTestSuite/FuncSignaturePassVerifier.h"
        "tests/EzTripleTestSuite/LegalizeCallActionVerifier.h"
        "tests/EzTripleTestSuite/LegalizeReturnActionVerifier.h"
        "tests/EzTripleTestSuite/PromoteScalarActionVerifier.h"

        "tests/EzTripleTestSuite/ExpandScalarActionVerifier.cpp"
        "tests/EzTripleTestSuite/EzTripleTestSuite.cpp"
        "tests/EzTripleTestSuite/FuncSignaturePassVerifier.cpp"
        "tests/EzTripleTestSuite/LegalizeCallActionVerifier.cpp"
        "tests/EzTripleTestSuite/LegalizeReturnActionVerifier.cpp"
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
            INCLUDED_DIRS "tests/EzTripleTestSuite"
    )

    EzPacker_AddTest(
            "T_LegalizeCallAction"
            "EzTripleTestSuite"
            ""
            "tests/T_LegalizeCallAction.cpp"
    )

    EzPacker_AddTest(
            "T_LegalizeReturnAction"
            "EzTripleTestSuite"
            ""
            "tests/T_LegalizeReturnAction.cpp"
    )

    EzPacker_AddTest(
            "T_PromoteScalarAction"
            "EzTripleTestSuite"
            ""
            "tests/T_PromoteScalarAction.cpp"
    )

    EzPacker_AddTest(
            "T_FunctionSignatureLegalizerPass"
            "EzTripleTestSuite"
            ""
            "tests/T_FunctionSignatureLegalizerPass.cpp"
    )
endif ()
