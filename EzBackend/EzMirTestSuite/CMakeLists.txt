set(MIRTESTSUITE_FILES_LIST
        "include/EzMirTestSuite.h"
        "include/Verifiers/ClassOffsetResolverVerifier.h"
        "include/Verifiers/CodeFlowPassVerifier.h"
        "include/Verifiers/LivenessPassVerifier.h"
        "include/Verifiers/MirCoreVerifiers.h"
        "include/Verifiers/RelativeReferenceLowererVerifier.h"

        "src/EzMirTestSuite.cpp"
        "src/Verifiers/ClassOffsetResolverVerifier.cpp"
        "src/Verifiers/CodeFlowPassVerifier.cpp"
        "src/Verifiers/LivenessPassVerifier.cpp"
        "src/Verifiers/MirCoreVerifiers.cpp"
        "src/Verifiers/RelativeReferenceLowererVerifier.cpp"
)

EzCMK_ConfigureLib(NAME "EzMirTestSuite"
        FILE_LIST ${MIRTESTSUITE_FILES_LIST}
        PRECOMPILED_HEADER "include/Verifiers/MirCoreVerifiers.h"
        LINKED_LIBRARIES "EzTestTriple"
        INCLUDED_DIRS "include"
)

EzPacker_AddTest(
        "T_Operand"
        "EzMirTestSuite"
        ""
        "tests/T_Operand.cpp"
)

EzPacker_AddTest(
        "T_Instruction"
        "EzMirTestSuite"
        ""
        "tests/T_Instruction.cpp"
)

EzPacker_AddTest(
        "T_Block"
        "EzMirTestSuite"
        ""
        "tests/T_Block.cpp"
)

EzPacker_AddTest(
        "T_Class"
        "EzMirTestSuite"
        ""
        "tests/T_Class.cpp"
)

EzPacker_AddTest(
        "T_ClassOffsetResolver"
        "EzMirTestSuite"
        ""
        "tests/T_ClassOffsetResolver.cpp"
)

EzPacker_AddTest(
        "T_Function"
        "EzMirTestSuite"
        ""
        "tests/T_Function.cpp"
)

EzPacker_AddTest(
        "T_GlobalVar"
        "EzMirTestSuite"
        ""
        "tests/T_GlobalVar.cpp"
)

EzPacker_AddTest(
        "T_CodeFlowPass"
        "EzMirTestSuite"
        ""
        "tests/T_CodeFlowPass.cpp"
)

EzPacker_AddTest(
        "T_LivenessAnalysis"
        "EzMirTestSuite"
        ""
        "tests/T_LivenessAnalysis.cpp"
)

EzPacker_AddTest(
        "T_RelativeReferenceLowerer"
        "EzMirTestSuite"
        ""
        "tests/T_RelativeReferenceLowerer.cpp"
)