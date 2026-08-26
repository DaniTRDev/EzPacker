set(EZDSLTESTSUITE_FILES_LIST
        "include/EzDslTestSuite.h"

        "src/EzDslTestSuite.cpp"
)

EzCMK_ConfigureLib(NAME "EzDslTestSuite"
        FILE_LIST ${EZDSLTESTSUITE_FILES_LIST}
        PRECOMPILED_HEADER ""
        LINKED_LIBRARIES "EzDsl;EzTriple;gtest;GTest::gtest_main"
        INCLUDED_DIRS "include"
)

EzPacker_AddTest(
        "T_EzDslEndToEndTargetPipeline"
        "EzDslTestSuite"
        ""
        "tests/T_EzDslEndToEndTargetPipeline.cpp"
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
        "T_EzDslCli_GenRegisterBanks"
        "EzDslTestSuite"
        ""
        "tests/T_EzDslCli_GenRegisterBanks.cpp"
)

EzPacker_AddTest(
        "T_EzDslCli_GenTargetInst"
        "EzDslTestSuite"
        ""
        "tests/T_EzDslCli_GenTargetInst.cpp"
)

EzPacker_AddTest(
        "T_EzDslCli_GenTargetTypeLayout"
        "EzDslTestSuite"
        ""
        "tests/T_EzDslCli_GenTargetTypeLayout.cpp"
)

EzPacker_AddTest(
        "T_EzDslCli_GenLegalizer"
        "EzDslTestSuite"
        ""
        "tests/T_EzDslCli_GenLegalizer.cpp"
)

EzPacker_AddTest(
        "T_EzDslCli_GenISelTable"
        "EzDslTestSuite"
        ""
        "tests/T_EzDslCli_GenISelTable.cpp"
)

EzPacker_AddTest(
        "T_EzDslCli_GenCallingConv"
        "EzDslTestSuite"
        ""
        "tests/T_EzDslCli_GenCallingConv.cpp"
)

EzPacker_AddTest(
        "T_EzDslCli_GenTargetDesc"
        "EzDslTestSuite"
        ""
        "tests/T_EzDslCli_GenTargetDesc.cpp"
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
        "T_Sema_CallingConvPass"
        "EzDslTestSuite"
        ""
        "tests/T_Sema_CallingConvPass.cpp"
)


EzPacker_AddTest(
        "T_Sema_InstSelPass"
        "EzDslTestSuite"
        ""
        "tests/T_Sema_InstSelPass.cpp"
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
        "T_Sema_TargetDefPass"
        "EzDslTestSuite"
        ""
        "tests/T_Sema_TargetDefPass.cpp"
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