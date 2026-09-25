set(EZTRIPLETESTSUITE_FILES_LIST
        "include/EzTripleTestSuite.h"
        "src/EzTripleTestSuite.cpp"
)

EzCMK_ConfigureLib(NAME "EzTripleTestSuite"
        FILE_LIST ${EZTRIPLETESTSUITE_FILES_LIST}
        PRECOMPILED_HEADER ""
        LINKED_LIBRARIES "EzTriple;EzMir;EzCore;EzCodeEmitter;EzTargetsX86_64;gtest;GTest::gtest_main"
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

EzPacker_AddTest(
        "T_MirRegisterAllocator"
        "EzTripleTestSuite"
        ""
        "tests/T_MirRegisterAllocator.cpp"
)

EzPacker_AddTest(
        "T_X86_64TargetDesc"
        "EzTripleTestSuite"
        ""
        "tests/T_X86_64TargetDesc.cpp"
)

EzPacker_AddTest(
        "T_X86_64RelocationResolver"
        "EzTripleTestSuite"
        ""
        "tests/T_X86_64RelocationResolver.cpp"
)

EzPacker_AddTest(
        "T_TargetExtensionSet"
        "EzTripleTestSuite"
        ""
        "tests/T_TargetExtensionSet.cpp"
)

EzPacker_AddTest(
        "T_MirRegisterCoalescer"
        "EzTripleTestSuite"
        ""
        "tests/T_MirRegisterCoalescer.cpp"
)



