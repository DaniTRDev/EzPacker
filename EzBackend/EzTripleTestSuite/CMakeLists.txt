set(TRIPLETESTSUITE_FILES_LIST
        "include/EzTripleTestExpansionRecipe.h"
        "include/EzTripleTestFrameLowerer.h"
        "include/EzTripleTestInstructionSet.h"
        "include/EzTripleTestLegalizer.h"
        "include/EzTripleTestSelector.h"
        "include/EzTripleTestSuite.h"
        "include/EzTripleTestTargetDescriptor.h"
        "include/Verifiers/CallAbiLowererVerifier.h"
        "include/Verifiers/ExpandScalarActionVerifier.h"
        "include/Verifiers/FrameLowererPassVerifier.h"
        "include/Verifiers/FuncSignaturePassVerifier.h"
        "include/Verifiers/FunctionParametersAbiLowererVerifier.h"
        "include/Verifiers/InstructionSelectorPassVerifier.h"
        "include/Verifiers/LegalizeCallActionVerifier.h"
        "include/Verifiers/LegalizeReturnActionVerifier.h"
        "include/Verifiers/PromoteScalarActionVerifier.h"
        "include/Verifiers/RegisterAllocatorPassVerifier.h"
        "include/Verifiers/ReturnAbiLowererVerifier.h"

        "src/EzTripleTestFrameLowerer.cpp"
        "src/EzTripleTestLegalizer.cpp"
        "src/EzTripleTestSelector.cpp"
        "src/EzTripleTestSuite.cpp"
        "src/EzTripleTestTargetDescriptor.cpp"
        "src/Verifiers/CallAbiLowererVerifier.cpp"
        "src/Verifiers/ExpandScalarActionVerifier.cpp"
        "src/Verifiers/FrameLowererPassVerifier.cpp"
        "src/Verifiers/FuncSignaturePassVerifier.cpp"
        "src/Verifiers/FunctionParametersAbiLowererVerifier.cpp"
        "src/Verifiers/InstructionSelectorPassVerifier.cpp"
        "src/Verifiers/LegalizeCallActionVerifier.cpp"
        "src/Verifiers/LegalizeReturnActionVerifier.cpp"
        "src/Verifiers/PromoteScalarActionVerifier.cpp"
        "src/Verifiers/RegisterAllocatorPassVerifier.cpp"
        "src/Verifiers/ReturnAbiLowererVerifier.cpp"
)

EzCMK_ConfigureLib(NAME "EzTripleTestSuite"
        FILE_LIST ${TRIPLETESTSUITE_FILES_LIST}
        PRECOMPILED_HEADER "include/EzTripleTestSuite.h"
        LINKED_LIBRARIES "EzTriple;EzMirTestSuite"
        INCLUDED_DIRS "include/"
)

EzPacker_AddTest(
        "T_CallAbiLowerer"
        "EzTripleTestSuite"
        ""
        "tests/T_CallAbiLowerer.cpp"
)

EzPacker_AddTest(
        "T_FrameLowererPass"
        "EzTripleTestSuite"
        ""
        "tests/T_FrameLowererPass.cpp"
)

EzPacker_AddTest(
        "T_FunctionParametersAbiLowerer"
        "EzTripleTestSuite"
        ""
        "tests/T_FunctionParametersAbiLowerer.cpp"
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
        "T_RegisterAllocatorPass"
        "EzTripleTestSuite"
        ""
        "tests/T_RegisterAllocatorPass.cpp"
)

EzPacker_AddTest(
        "T_ReturnAbiLowerer"
        "EzTripleTestSuite"
        ""
        "tests/T_ReturnAbiLowerer.cpp"
)

EzPacker_AddTest(
        "T_ScalarExpansionAction"
        "EzTripleTestSuite"
        ""
        "tests/T_ScalarExpansionAction.cpp"
)