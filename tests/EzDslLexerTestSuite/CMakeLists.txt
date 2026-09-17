set(EZDSLLEXERTESTSUITE_FILES_LIST
        "include/EzDslLexerTestSuite.h"

        "src/EzDslLexerTestSuite.cpp"
)

EzCMK_ConfigureLib(NAME "EzDslLexerTestSuite"
        FILE_LIST ${EZDSLLEXERTESTSUITE_FILES_LIST}
        PRECOMPILED_HEADER ""
        LINKED_LIBRARIES "EzDslLexer;gtest;GTest::gtest_main"
        INCLUDED_DIRS "include"
)

EzPacker_AddTest(
        "T_IrInstructionDefLang"
        "EzDslLexerTestSuite"
        ""
        "tests/T_IrInstructionDefLang.cpp"
)

EzPacker_AddTest(
        "T_LegalizeActionDefLang"
        "EzDslLexerTestSuite"
        ""
        "tests/T_LegalizeActionDefLang.cpp"
)

EzPacker_AddTest(
        "T_LegalizeRuleDefLang"
        "EzDslLexerTestSuite"
        ""
        "tests/T_LegalizeRuleDefLang.cpp"
)

EzPacker_AddTest(
        "T_TypeDefLang"
        "EzDslLexerTestSuite"
        ""
        "tests/T_TypeDefLang.cpp"
)

EzPacker_AddTest(
        "T_TargetInstDefLang"
        "EzDslLexerTestSuite"
        ""
        "tests/T_TargetInstDefLang.cpp"
)

EzPacker_AddTest(
        "T_InstructionSelectDefLang"
        "EzDslLexerTestSuite"
        ""
        "tests/T_InstructionSelectDefLang.cpp"
)

EzPacker_AddTest(
        "T_CallingConvDefLang"
        "EzDslLexerTestSuite"
        ""
        "tests/T_CallingConvDefLang.cpp"
)