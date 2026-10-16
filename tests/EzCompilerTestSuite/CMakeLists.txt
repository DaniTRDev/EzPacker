set(EZCOMPILERTESTSUITE_FILES_LIST
        "include/EzCompilerTestSuite.h"
        "src/EzCompilerTestSuite.cpp"
)

EzCMK_ConfigureLib(NAME "EzCompilerTestSuite"
        FILE_LIST ${EZCOMPILERTESTSUITE_FILES_LIST}
        PRECOMPILED_HEADER ""
        LINKED_LIBRARIES "EzCompilerLib;EzTargetsX86_64Registration;EzTriple;EzMir;EzCore;EzCodeEmitter;EzLinkerLib;EzExceptionRuntime;chibicc_frontend;gtest;GTest::gtest_main"
        INCLUDED_DIRS "include;../../EzCompiler/include;../../EzCodeEmitter/include;../../EzTriple/include;../../EzMir/include;../../EzCore/include;../../EzLinker/include;../../EzLinker/runtime;${CMAKE_SOURCE_DIR}/EzTargets/X86_64/Registration/include;${chibicc_SOURCE_DIR};${chibicc_SOURCE_DIR}/shims"
)

EzPacker_AddTest(
        "T_ChibiccWrapper"
        "EzCompilerTestSuite"
        ""
        "tests/T_ChibiccWrapper.cpp"
)

EzPacker_AddTest(
        "T_CompilerCommandLineParser"
        "EzCompilerTestSuite"
        ""
        "tests/T_CommandLineParser.cpp"
)

EzPacker_AddTest(
        "T_CompilerTargetResolver"
        "EzCompilerTestSuite"
        ""
        "tests/T_TargetResolver.cpp"
)

EzPacker_AddTest(
        "T_CompilerEndToEndCompilation"
        "EzCompilerTestSuite"
        ""
        "tests/T_EndToEndCompilation.cpp"
)

EzPacker_AddTest(
        "T_CompilerOptimizationPipeline"
        "EzCompilerTestSuite"
        ""
        "tests/T_OptimizationPipeline.cpp"
)

EzPacker_AddTest(
        "T_CompilerVerificationPipeline"
        "EzCompilerTestSuite"
        ""
        "tests/T_VerificationPipeline.cpp"
)

EzPacker_AddTest(
        "T_CFrontendAdapter"
        "EzCompilerTestSuite"
        ""
        "tests/T_CFrontendAdapter.cpp"
)

EzPacker_AddTest(
        "T_CFrontendEndToEnd"
        "EzCompilerTestSuite"
        ""
        "tests/T_CFrontendEndToEnd.cpp"
)


