set(EZDSLSEMATESTSUITE_FILES_LIST
        "include/EzDslSemaTestSuite.h"

        "src/EzDslSemaTestSuite.cpp"
)

EzCMK_ConfigureLib(NAME "EzDslSemaTestSuite"
        FILE_LIST ${EZDSLSEMATESTSUITE_FILES_LIST}
        PRECOMPILED_HEADER ""
        LINKED_LIBRARIES "EzDslSema;gtest;GTest::gtest_main"
        INCLUDED_DIRS "include"
)

EzPacker_AddTest(
        "T_IrInstructionDefLang"
        "EzDslSemaTestSuite"
        ""
        "tests/T_IrInstructionDefLang.cpp"
)

EzPacker_AddTest(
        "T_TypeDefLang"
        "EzDslSemaTestSuite"
        ""
        "tests/T_TypeDefLang.cpp"
)

EzPacker_AddTest(
        "T_LegalizeActionDefLang"
        "EzDslSemaTestSuite"
        ""
        "tests/T_LegalizeActionDefLang.cpp"
)

EzPacker_AddTest(
        "T_CallingConvDefLang"
        "EzDslSemaTestSuite"
        ""
        "tests/T_CallingConvDefLang.cpp"
)

