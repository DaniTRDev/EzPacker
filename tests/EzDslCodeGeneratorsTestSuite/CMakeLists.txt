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

EzPacker_AddTest(
        "T_CppTargetInstructionGenerator"
        "EzDslCodeGeneratorsTestSuite"
        "EzDslCodeGenerators"
        "tests/T_CppTargetInstructionGenerator.cpp"
)

EzPacker_AddTest(
        "T_CppTargetEncodingGenerator"
        "EzDslCodeGeneratorsTestSuite"
        "EzDslCodeGenerators"
        "tests/T_CppTargetEncodingGenerator.cpp"
)

EzPacker_AddTest(
        "T_CppInstructionSelectorGenerator"
        "EzDslCodeGeneratorsTestSuite"
        "EzDslCodeGenerators"
        "tests/T_CppInstructionSelectorGenerator.cpp"
)

EzPacker_AddTest(
        "T_CppCallingConvGenerator"
        "EzDslCodeGeneratorsTestSuite"
        "EzDslCodeGenerators"
        "tests/T_CppCallingConvGenerator.cpp"
)

EzPacker_AddTest(
        "T_CppRegisterInfoGenerator"
        "EzDslCodeGeneratorsTestSuite"
        "EzDslCodeGenerators"
        "tests/T_CppRegisterInfoGenerator.cpp"
)

EzPacker_AddTest(
        "T_CppTargetDescGenerator"
        "EzDslCodeGeneratorsTestSuite"
        "EzDslCodeGenerators"
        "tests/T_CppTargetDescGenerator.cpp"
)

