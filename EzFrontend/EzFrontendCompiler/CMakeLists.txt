
set(EZFRONTEND_COMPILATION_UNIT_FILES_LIST
        "include/EzFrontendCompilerCommon.h"
        "include/FrontendCompilationUnit.h"
        "include/FrontendCompilationUnitPhase.h"
        "include/FrontendCompilerDriver.h"
        "include/CompilationPhases/AstLowering.h"
        "include/CompilationPhases/Parsing.h"
        "include/CompilationPhases/SemanticAnalysis.h"
        "include/CompilationPhases/Tokenization.h"

        "src/FrontendCompilationUnit.cpp"
        "src/FrontendCompilerDriver.cpp"
        "src/CompilationPhases/AstLowering.cpp"
        "src/CompilationPhases/Parsing.cpp"
        "src/CompilationPhases/SemanticAnalysis.cpp"
        "src/CompilationPhases/Tokenization.cpp"
)

EzCMK_ConfigureLib(NAME "EzFrontendCompiler"
        FILE_LIST ${EZFRONTEND_COMPILATION_UNIT_FILES_LIST}
        PRECOMPILED_HEADER "include/EzFrontendCompilerCommon.h"
        LINKED_LIBRARIES "EzSemantics"
        INCLUDED_DIRS "include")

# ─────────────────────────────────────────────────────────────────────────────
#  EzFrontendCompiler Tests
# ─────────────────────────────────────────────────────────────────────────────

set(FC_FIXTURE
        "tests/FrontendCompilerTestFixture.h;tests/FrontendCompilerTestFixture.cpp"
)

EzPacker_AddTest(
        "FC_CompilationUnit"
        "EzFrontendCompiler;EzSemantics;EzLexer;EzMir;gtest_main"
        ""
        "tests/CompilationUnitTest.cpp;${FC_FIXTURE}"
)

EzPacker_AddTest(
        "FC_CompilerDriver"
        "EzFrontendCompiler;EzSemantics;EzLexer;EzMir;gtest_main"
        ""
        "tests/CompilerDriverTest.cpp;${FC_FIXTURE}"
)

EzPacker_AddTest(
        "FC_CompilerDriver_MultiFiles"
        "EzFrontendCompiler;EzSemantics;EzLexer;EzMir;gtest_main"
        ""
        "tests/CompilerDriverTestMultiFiles.cpp;${FC_FIXTURE}"
)

EzPacker_AddTest(
        "FC_TokenizationPhase"
        "EzFrontendCompiler;EzSemantics;EzLexer;EzMir;gtest_main"
        ""
        "tests/TokenizationPhaseTest.cpp;${FC_FIXTURE}"
)

EzPacker_AddTest(
        "FC_ParsingPhase"
        "EzFrontendCompiler;EzSemantics;EzLexer;EzMir;gtest_main"
        ""
        "tests/ParsingPhaseTest.cpp;${FC_FIXTURE}"
)

EzPacker_AddTest(
        "FC_SemanticAnalysisPhase"
        "EzFrontendCompiler;EzSemantics;EzLexer;EzMir;gtest_main"
        ""
        "tests/SemanticAnalysisPhaseTest.cpp;${FC_FIXTURE}"
)

EzPacker_AddTest(
        "FC_AstLoweringPhase"
        "EzFrontendCompiler;EzSemantics;EzLexer;EzMir;gtest_main"
        ""
        "tests/AstLoweringPhaseTest.cpp;${FC_FIXTURE}"
)
