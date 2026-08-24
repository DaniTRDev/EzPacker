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