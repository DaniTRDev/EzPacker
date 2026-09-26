set(EZMIRTESTSUITE_FILES_LIST
        "include/EzMirTestSuite.h"
        "include/EzMirTestSuiteCallingConv.h"

        "src/EzMirTestSuite.cpp"
        "src/EzMirTestSuiteCallingConv.cpp"
)

EzCMK_ConfigureLib(NAME "EzMirTestSuite"
        FILE_LIST ${EZMIRTESTSUITE_FILES_LIST}
        PRECOMPILED_HEADER ""
        LINKED_LIBRARIES "EzMir;gtest;GTest::gtest_main"
        INCLUDED_DIRS "include"
)

EzPacker_AddTest(
        "T_Block"
        "EzMirTestSuite"
        ""
        "tests/T_Block.cpp"
)

EzPacker_AddTest(
        "T_CodeFlowPass"
        "EzMirTestSuite"
        ""
        "tests/T_CodeFlowPass.cpp"
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
        "T_Instruction"
        "EzMirTestSuite"
        ""
        "tests/T_Instruction.cpp"
)

EzPacker_AddTest(
        "T_LivenessAnalysis"
        "EzMirTestSuite"
        ""
        "tests/T_LivenessAnalysis.cpp"
)

EzPacker_AddTest(
        "T_MirFunctionRegisterInfo"
        "EzMirTestSuite"
        ""
        "tests/T_MirFunctionRegisterInfo.cpp"
)

EzPacker_AddTest(
        "T_NonSsaToSsa"
        "EzMirTestSuite"
        ""
        "tests/T_NonSsaToSsa.cpp"
)

EzPacker_AddTest(
        "T_Operand"
        "EzMirTestSuite"
        ""
        "tests/T_Operand.cpp"
)

EzPacker_AddTest(
        "T_MirParser"
        "EzMirTestSuite"
        ""
        "tests/T_MirParser.cpp"
)

EzPacker_AddTest(
        "T_MirRegisterDescriptor"
        "EzMirTestSuite"
        ""
        "tests/T_MirRegisterDescriptor.cpp"
)

EzPacker_AddTest(
        "T_MirPeepholePass"
        "EzMirTestSuite"
        ""
        "tests/T_MirPeepholePass.cpp"
)

EzPacker_AddTest(
        "T_MirVerifierPass"
        "EzMirTestSuite"
        ""
        "tests/T_MirVerifierPass.cpp"
)