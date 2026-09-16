set(EZDSLCODEGENERATORSTESTSUITE_FILES_LIST
        "include/EzDslCodeGeneratorsTestSuite.h"

        "src/EzDslCodeGeneratorsTestSuite.cpp"
)

EzCMK_ConfigureLib(NAME "EzDslCodeGeneratorsTestSuite"
        FILE_LIST ${EZDSLCODEGENERATORSTESTSUITE_FILES_LIST}
        PRECOMPILED_HEADER ""
        LINKED_LIBRARIES "EzDslCodeGenerators;EzDslSema;gtest;GTest::gtest_main"
        INCLUDED_DIRS "include"
)

EzPacker_AddTest(
        "T_CppSourceEmitter"
        "EzDslCodeGeneratorsTestSuite"
        "EzDslCodeGenerators"
        "tests/T_CppSourceEmitter.cpp"
)

EzPacker_AddTest(
        "T_CodeGenerator"
        "EzDslCodeGeneratorsTestSuite"
        "EzDslCodeGenerators"
        "tests/T_CodeGenerator.cpp"
)

EzPacker_AddTest(
        "T_CppMirInstructionGenerator"
        "EzDslCodeGeneratorsTestSuite"
        "EzDslCodeGenerators"
        "tests/T_CppMirInstructionGenerator.cpp"
)

EzPacker_AddTest(
        "T_CppMirTypeTableGenerator"
        "EzDslCodeGeneratorsTestSuite"
        "EzDslCodeGenerators"
        "tests/T_CppMirTypeTableGenerator.cpp"
)

EzPacker_AddTest(
        "T_CppLegalizerGenerator"
        "EzDslCodeGeneratorsTestSuite"
        "EzDslCodeGenerators"
        "tests/T_CppLegalizerGenerator.cpp"
)

EzPacker_AddTest(
        "T_CppLegalizeRuleGenerator"
        "EzDslCodeGeneratorsTestSuite"
        "EzDslCodeGenerators"
        "tests/T_CppLegalizeRuleGenerator.cpp"
)
