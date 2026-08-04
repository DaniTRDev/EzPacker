set(EZTRIPLE_FILES_LIST
        "include/EzTriple.h"
        "include/EzTripleCommon.h"
        "include/AbiLowerer/AbiLowerer.h"
        "include/AbiLowerer/FunctionAbiLowererPass.h"
        "include/Descriptors/TargetDesc.h"
        "include/ExpansionRecipe/ExpansionRecipe.h"
        "include/InstructionSelector/Actions/ManualSelectAction.h"
        "include/InstructionSelector/InstructionSelectionPredicates.h"
        "include/InstructionSelector/InstructionSelectionRuleBuilder.h"
        "include/InstructionSelector/MirInstructionSelector.h"
        "include/InstructionSelector/MirInstructionSelectorPass.h"
        "include/Legalizer/Actions/ExpandScalarAction.h"
        "include/Legalizer/Actions/LegalAction.h"
        "include/Legalizer/Actions/LegalizeCallAction.h"
        "include/Legalizer/Actions/LegalizeReturnAction.h"
        "include/Legalizer/Actions/PromoteScalarAction.h"
        "include/Legalizer/LegalizeRuleBuilder.h"
        "include/Legalizer/MirBlockLegalizerPass.h"
        "include/Legalizer/MirFunctionSignatureLegalizerPass.h"
        "include/Legalizer/MirLegalizer.h"

        "src/AbiLowerer/AbiLowerer.cpp"
        "src/AbiLowerer/FunctionAbiLowererPass.cpp"
        "src/InstructionSelector/Actions/ManualSelectAction.cpp"
        "src/InstructionSelector/InstructionSelectionPredicates.cpp"
        "src/InstructionSelector/InstructionSelectionRuleBuilder.cpp"
        "src/InstructionSelector/MirInstructionSelector.cpp"
        "src/InstructionSelector/MirInstructionSelectorPass.cpp"
        "src/Legalizer/Actions/ExpandScalarAction.cpp"
        "src/Legalizer/Actions/LegalAction.cpp"
        "src/Legalizer/Actions/LegalizeCallAction.cpp"
        "src/Legalizer/Actions/LegalizeReturnAction.cpp"
        "src/Legalizer/Actions/PromoteScalarAction.cpp"
        "src/Legalizer/LegalizeRuleBuilder.cpp"
        "src/Legalizer/MirBlockLegalizerPass.cpp"
        "src/Legalizer/MirFunctionSignatureLegalizerPass.cpp"
        "src/Legalizer/MirLegalizer.cpp"
)

set(TRIPLETESTSUITE_FILES_LIST
        "tests/EzTripleTestSuite/CallAbiLowererVerifier.h"
        "tests/EzTripleTestSuite/ExpandScalarActionVerifier.h"
        "tests/EzTripleTestSuite/EzTripleTestExpansionRecipe.h"
        "tests/EzTripleTestSuite/EzTripleTestInstructionSet.h"
        "tests/EzTripleTestSuite/EzTripleTestLegalizer.h"
        "tests/EzTripleTestSuite/EzTripleTestSelector.h"
        "tests/EzTripleTestSuite/EzTripleTestSuite.h"
        "tests/EzTripleTestSuite/EzTripleTestTargetDescriptor.h"
        "tests/EzTripleTestSuite/FuncSignaturePassVerifier.h"
        "tests/EzTripleTestSuite/FunctionParametersAbiLowererVerifier.h"
        "tests/EzTripleTestSuite/InstructionSelectorPassVerifier.h"
        "tests/EzTripleTestSuite/LegalizeCallActionVerifier.h"
        "tests/EzTripleTestSuite/LegalizeReturnActionVerifier.h"
        "tests/EzTripleTestSuite/PromoteScalarActionVerifier.h"
        "tests/EzTripleTestSuite/ReturnAbiLowererVerifier.h"

        "tests/EzTripleTestSuite/CallAbiLowererVerifier.cpp"
        "tests/EzTripleTestSuite/ExpandScalarActionVerifier.cpp"
        "tests/EzTripleTestSuite/EzTripleTestLegalizer.cpp"
        "tests/EzTripleTestSuite/EzTripleTestSelector.cpp"
        "tests/EzTripleTestSuite/EzTripleTestSuite.cpp"
        "tests/EzTripleTestSuite/FuncSignaturePassVerifier.cpp"
        "tests/EzTripleTestSuite/FunctionParametersAbiLowererVerifier.cpp"
        "tests/EzTripleTestSuite/InstructionSelectorPassVerifier.cpp"
        "tests/EzTripleTestSuite/LegalizeCallActionVerifier.cpp"
        "tests/EzTripleTestSuite/LegalizeReturnActionVerifier.cpp"
        "tests/EzTripleTestSuite/PromoteScalarActionVerifier.cpp"
        "tests/EzTripleTestSuite/ReturnAbiLowererVerifier.cpp"
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

    EzPacker_AddTest(
            "T_InstructionSelectorPass"
            "EzTripleTestSuite"
            ""
            "tests/T_InstructionSelectorPass.cpp"
    )

    EzPacker_AddTest(
            "T_ScalarExpansionAction"
            "EzTripleTestSuite"
            ""
            "tests/T_ScalarExpansionAction.cpp"
    )

    EzPacker_AddTest(
            "T_ReturnAbiLowerer"
            "EzTripleTestSuite"
            ""
            "tests/T_ReturnAbiLowerer.cpp"
    )

    EzPacker_AddTest(
            "T_CallAbiLowerer"
            "EzTripleTestSuite"
            ""
            "tests/T_CallAbiLowerer.cpp"
    )

    EzPacker_AddTest(
            "T_FunctionParametersAbiLowerer"
            "EzTripleTestSuite"
            ""
            "tests/T_FunctionParametersAbiLowerer.cpp"
    )
endif ()
