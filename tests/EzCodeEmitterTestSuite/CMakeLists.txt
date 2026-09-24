set(EZCODEEMITTERTESTSUITE_FILES_LIST
        "include/EzCodeEmitterTestSuite.h"
        "src/EzCodeEmitterTestSuite.cpp"
)

EzCMK_ConfigureLib(NAME "EzCodeEmitterTestSuite"
        FILE_LIST ${EZCODEEMITTERTESTSUITE_FILES_LIST}
        PRECOMPILED_HEADER ""
        LINKED_LIBRARIES "EzCodeEmitter;EzTriple;EzMir;EzCore;EzTargetsX86_64;gtest;GTest::gtest_main"
        INCLUDED_DIRS "include"
)

EzPacker_AddTest(
        "T_BranchRelaxation"
        "EzCodeEmitterTestSuite"
        ""
        "tests/T_BranchRelaxation.cpp"
)

EzPacker_AddTest(
        "T_ObjectFormatWriters"
        "EzCodeEmitterTestSuite"
        ""
        "tests/T_ObjectFormatWriters.cpp"
)

EzPacker_AddTest(
        "T_X86_64CodeEmitter"
        "EzCodeEmitterTestSuite"
        ""
        "tests/T_X86_64CodeEmitter.cpp"
)

EzPacker_AddTest(
        "T_StressAndSanitizers"
        "EzCodeEmitterTestSuite"
        ""
        "tests/T_StressAndSanitizers.cpp"
)

EzPacker_AddTest(
        "T_X86_64Encoding"
        "EzCodeEmitterTestSuite"
        ""
        "tests/T_X86_64Encoding.cpp"
)
