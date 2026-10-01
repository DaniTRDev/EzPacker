set(EZLINKERTESTSUITE_FILES_LIST
        "include/EzLinkerTestSuite.h"
        "src/EzLinkerTestSuite.cpp"
)

EzCMK_ConfigureLib(NAME "EzLinkerTestSuite"
        FILE_LIST ${EZLINKERTESTSUITE_FILES_LIST}
        PRECOMPILED_HEADER ""
        LINKED_LIBRARIES "EzLinkerLib;EzExceptionRuntime;EzCore;gtest;GTest::gtest_main"
        INCLUDED_DIRS "include;../../EzLinker/include;../../EzLinker/runtime;../../EzCore/include"
)

EzPacker_AddTest(
        "T_SystemLinkerDetector"
        "EzLinkerTestSuite"
        ""
        "tests/T_SystemLinkerDetector.cpp"
)

EzPacker_AddTest(
        "T_EzLinkerDriver"
        "EzLinkerTestSuite"
        ""
        "tests/T_EzLinkerDriver.cpp"
)

EzPacker_AddTest(
        "T_EzExceptionRuntime"
        "EzLinkerTestSuite"
        ""
        "tests/T_EzExceptionRuntime.cpp"
)
