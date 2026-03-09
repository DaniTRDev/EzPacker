
set(EZFRONTEND_COMPILATION_UNIT_FILES_LIST
        "include/EzFrontendCompilerCommon.h"
        "include/FrontendCompilationUnit.h"
        "include/FrontendCompilationUnitPhase.h"
        "include/FrontendCompilerDriver.h"
        "include/CompilationPhases/AstLowering.h"
        "include/CompilationPhases/IncludePhase.h"
        "include/CompilationPhases/Parsing.h"
        "include/CompilationPhases/SemanticAnalysis.h"
        "include/CompilationPhases/Tokenization.h"
        "include/CompilationPhases/Semantic/SymbolAndTypeResolver.h"
        "include/CompilationPhases/Semantic/SymbolDefinition.h"
        "include/CompilationPhases/Semantic/TypeCheck.h"
        "include/IncludeVisitor/IncludeVisitor.h"
        
        "src/FrontendCompilationUnit.cpp"
        "src/FrontendCompilerDriver.cpp"
        "src/CompilationPhases/AstLowering.cpp"
        "src/CompilationPhases/IncludePhase.cpp"
        "src/CompilationPhases/Parsing.cpp"
        "src/CompilationPhases/SemanticAnalysis.cpp"
        "src/CompilationPhases/Tokenization.cpp"
        "src/CompilationPhases/Semantic/SymbolAndTypeResolver.cpp"
        "src/CompilationPhases/Semantic/SymbolDefinition.cpp"
        "src/CompilationPhases/Semantic/TypeCheck.cpp"
        "src/IncludeVisitor/IncludeVisitor.cpp"
)

EzCMK_ConfigureLib(NAME "EzFrontendCompiler"
        FILE_LIST ${EZFRONTEND_COMPILATION_UNIT_FILES_LIST}
        PRECOMPILED_HEADER "include/EzFrontendCompilerCommon.h"
        LINKED_LIBRARIES "EzSemantics"
        INCLUDED_DIRS "include")

# ─────────────────────────────────────────────────────────────────────────────
#  EzFrontendCompiler tests
# ─────────────────────────────────────────────────────────────────────────────

set(FC_FIXTURE
        "tests/FrontendCompilerTestFixture.h;tests/FrontendCompilerTestFixture.cpp"
)

EzPacker_AddTest(
        "FC_CompilationUnit"
        "EzFrontendCompiler;EzSemantics;EzLexer;EzMir;gtest_main"
        ""
        "tests/FrontendCompilationUnitTest.cpp;${FC_FIXTURE}"
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

EzPacker_AddTest(
        "FC_IncludePhase"
        "EzFrontendCompiler;EzSemantics;EzLexer;EzMir;gtest_main"
        ""
        "tests/IncludePhaseTest.cpp;${FC_FIXTURE}"
)

EzPacker_AddTest(
        "FC_CompilerDriver"
        "EzFrontendCompiler;EzSemantics;EzLexer;EzMir;gtest_main"
        ""
        "tests/CompilerDriverTest.cpp;${FC_FIXTURE}"
)
