set(EZDSLTESTSUITE_FILES_LIST
        "include/EzDslTestSuite.h"

        "src/EzDslTestSuite.cpp"
)

EzCMK_ConfigureLib(NAME "EzDslTestSuite"
        FILE_LIST ${EZDSLTESTSUITE_FILES_LIST}
        PRECOMPILED_HEADER ""
        LINKED_LIBRARIES "EzDsl;gtest;GTest::gtest_main"
        INCLUDED_DIRS "include"
)

EzPacker_AddTest(
        "T_CallingConvDefLang"
        "EzDslTestSuite"
        ""
        "tests/T_CallingConvDefLang.cpp"
)

EzPacker_AddTest(
        "T_EzDslCli_GenMirInstruction"
        "EzDslTestSuite"
        ""
        "tests/T_EzDslCli_GenMirInstruction.cpp"
)

EzPacker_AddTest(
        "T_EzDslCli_GenTypeTable"
        "EzDslTestSuite"
        ""
        "tests/T_EzDslCli_GenTypeTable.cpp"
)

EzPacker_AddTest(
        "T_InstructionDefLang"
        "EzDslTestSuite"
        ""
        "tests/T_InstructionDefLang.cpp"
)

EzPacker_AddTest(
        "T_InstructionSelDefLang"
        "EzDslTestSuite"
        ""
        "tests/T_InstructionSelDefLang.cpp"
)

EzPacker_AddTest(
        "T_IrInstructionDefLang"
        "EzDslTestSuite"
        ""
        "tests/T_IrInstructionDefLang.cpp"
)

EzPacker_AddTest(
        "T_LegalizeActionDefLang"
        "EzDslTestSuite"
        ""
        "tests/T_LegalizeActionDefLang.cpp"
)

EzPacker_AddTest(
        "T_LegalizeRuleDefLang"
        "EzDslTestSuite"
        ""
        "tests/T_LegalizeRuleDefLang.cpp"
)

EzPacker_AddTest(
        "T_Sema_IrInstructionPass"
        "EzDslTestSuite"
        ""
        "tests/T_Sema_IrInstructionPass.cpp"
)

EzPacker_AddTest(
        "T_Sema_LegalizeActionPass"
        "EzDslTestSuite"
        ""
        "tests/T_Sema_LegalizeActionPass.cpp"
)

EzPacker_AddTest(
        "T_Sema_LegalizeRulePass"
        "EzDslTestSuite"
        ""
        "tests/T_Sema_LegalizeRulePass.cpp"
)

EzPacker_AddTest(
        "T_Sema_RegisterBankPass"
        "EzDslTestSuite"
        ""
        "tests/T_Sema_RegisterBankPass.cpp"
)

EzPacker_AddTest(
        "T_Sema_TargetInstPass"
        "EzDslTestSuite"
        ""
        "tests/T_Sema_TargetInstPass.cpp"
)

EzPacker_AddTest(
        "T_TargetDefLang"
        "EzDslTestSuite"
        ""
        "tests/T_TargetDefLang.cpp"
)

EzPacker_AddTest(
        "T_TypeDefLang"
        "EzDslTestSuite"
        ""
        "tests/T_TypeDefLang.cpp"
)