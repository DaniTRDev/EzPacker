set(EZTRIPLETESTSUITE_FILES_LIST
        "include/EzTripleTestSuite.h"
        "src/EzTripleTestSuite.cpp"
)

EzCMK_ConfigureLib(NAME "EzTripleTestSuite"
        FILE_LIST ${EZTRIPLETESTSUITE_FILES_LIST}
        PRECOMPILED_HEADER ""
        LINKED_LIBRARIES "EzTriple;EzMir;EzCore;gtest;GTest::gtest_main"
        INCLUDED_DIRS "include"
)

EzPacker_AddTest(
        "T_MirLegalizer"
        "EzTripleTestSuite"
        ""
        "tests/T_MirLegalizer.cpp"
)

EzPacker_AddTest(
        "T_MirInstructionSelector"
        "EzTripleTestSuite"
        ""
        "tests/T_MirInstructionSelector.cpp"
)

EzPacker_AddTest(
        "T_MirAbiLowerer"
        "EzTripleTestSuite"
        ""
        "tests/T_MirAbiLowerer.cpp"
)

EzPacker_AddTest(
        "T_MirFrameLowerer"
        "EzTripleTestSuite"
        ""
        "tests/T_MirFrameLowerer.cpp"
)
