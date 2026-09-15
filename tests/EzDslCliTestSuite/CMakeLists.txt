set(EZDSLCLITESTSUITE_FILES_LIST
        "include/EzDslCliTestSuite.h"

        "src/EzDslCliTestSuite.cpp"
)

EzCMK_ConfigureLib(NAME "EzDslCliTestSuite"
        FILE_LIST ${EZDSLCLITESTSUITE_FILES_LIST}
        PRECOMPILED_HEADER ""
        LINKED_LIBRARIES "EzDslCliLib;EzDslCodeGenerators;EzDslSema;EzDslLexer;EzCore;argparse;gtest;GTest::gtest_main"
        INCLUDED_DIRS "include"
)

EzPacker_AddTest(
        "T_CommandLineParser"
        "EzDslCliTestSuite"
        "EzDslCliLib"
        "tests/T_CommandLineParser.cpp"
)

EzPacker_AddTest(
        "T_InfoDumper"
        "EzDslCliTestSuite"
        "EzDslCliLib"
        "tests/T_InfoDumper.cpp"
)

EzPacker_AddTest(
        "T_Driver"
        "EzDslCliTestSuite"
        "EzDslCliLib"
        "tests/T_Driver.cpp"
)

EzPacker_AddTest(
        "T_EzMirIntegration"
        "EzDslCliTestSuite"
        "EzDslCliLib"
        "tests/T_EzMirIntegration.cpp"
)
