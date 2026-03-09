set(EZSEMANTICS_FILES_LIST
        "include/BasicSemanticContext.h"
        "include/EzSemantics.h"
        "include/EzSemanticsCommon.h"
        "include/SemanticVisitor.h"
        "include/AstLowererVisitor/AstLowererVisitor.h"
        "include/AstLowererVisitor/BreakLowerer.h"
        "include/AstLowererVisitor/CodeScopeLowerer.h"
        "include/AstLowererVisitor/ConditionLowerer.h"
        "include/AstLowererVisitor/ContinueLowerer.h"
        "include/AstLowererVisitor/ForLowerer.h"
        "include/AstLowererVisitor/GenericLowerer.h"
        "include/AstLowererVisitor/IfLowerer.h"
        "include/AstLowererVisitor/ImmediateLowerer.h"
        "include/AstLowererVisitor/InstructionLowerer.h"
        "include/AstLowererVisitor/LabelLowerer.h"
        "include/AstLowererVisitor/LoweringContext.h"
        "include/AstLowererVisitor/MemoryLowerer.h"
        "include/AstLowererVisitor/ModuleLowerer.h"
        "include/AstLowererVisitor/SwitchLowerer.h"
        "include/AstLowererVisitor/VariableLowerer.h"
        "include/AstLowererVisitor/WhileLowerer.h"
        "include/Scope/Scope.h"
        "include/Scope/Symbol.h"
        "include/Scope/TypeTable.h"
        "include/SemanticAnnotations/DataTypeAnnotation.h"
        "include/SemanticAnnotations/ScopeAnnotation.h"
        "include/SemanticAnnotations/ScopedSymbolAnnotation.h"
        "include/SemanticAnnotations/SymbolAnnotation.h"
        "include/SemanticAnnotations/TypeCastAnnotation.h"
        "include/SymbolVisitors/SymbolAndTypeResolverVisitor.h"
        "include/SymbolVisitors/SymbolDefinitionVisitor.h"
        "include/SymbolVisitors/TypeCheckVisitor.h"

        "src/BasicSemanticContext.cpp"
        "src/SemanticVisitor.cpp"
        "src/AstLowererVisitor/AstLowererVisitor.cpp"
        "src/AstLowererVisitor/BreakLowerer.cpp"
        "src/AstLowererVisitor/CodeScopeLowerer.cpp"
        "src/AstLowererVisitor/ConditionLowerer.cpp"
        "src/AstLowererVisitor/ContinueLowerer.cpp"
        "src/AstLowererVisitor/ForLowerer.cpp"
        "src/AstLowererVisitor/IfLowerer.cpp"
        "src/AstLowererVisitor/ImmediateLowerer.cpp"
        "src/AstLowererVisitor/InstructionLowerer.cpp"
        "src/AstLowererVisitor/LabelLowerer.cpp"
        "src/AstLowererVisitor/LoweringContext.cpp"
        "src/AstLowererVisitor/MemoryLowerer.cpp"
        "src/AstLowererVisitor/ModuleLowerer.cpp"
        "src/AstLowererVisitor/SwitchLowerer.cpp"
        "src/AstLowererVisitor/VariableLowerer.cpp"
        "src/AstLowererVisitor/WhileLowerer.cpp"
        "src/Scope/Scope.cpp"
        "src/Scope/Symbol.cpp"
        "src/Scope/TypeTable.cpp"
        "src/SemanticAnnotations/DataTypeAnnotation.cpp"
        "src/SemanticAnnotations/ScopeAnnotation.cpp"
        "src/SemanticAnnotations/ScopedSymbolAnnotation.cpp"
        "src/SemanticAnnotations/SymbolAnnotation.cpp"
        "src/SemanticAnnotations/TypeCastAnnotation.cpp"
        "src/SymbolVisitors/SymbolAndTypeResolverVisitor.cpp"
        "src/SymbolVisitors/SymbolDefinitionVisitor.cpp"
        "src/SymbolVisitors/TypeCheckVisitor.cpp"
)

EzCMK_ConfigureLib(NAME "EzSemantics"
        FILE_LIST ${EZSEMANTICS_FILES_LIST}
        PRECOMPILED_HEADER "include/EzSemanticsCommon.h"
        LINKED_LIBRARIES "EzLexer;EzMir"
        INCLUDED_DIRS "include")

EzPacker_AddTest(
        "SymbolDefinitionVisitorTest"
        "EzSemantics;EzLexer;gtest_main"
        ""
        "tests/SymbolVisitors/SymbolDefinitionVisitorTest.cpp;tests/SymbolVisitors/SymbolDefinitionVisitorTestFixture.h;tests/SymbolVisitors/SymbolDefinitionVisitorTestFixture.cpp"
)

EzPacker_AddTest(
        "SymbolResolverVisitorTest"
        "EzSemantics;EzLexer;gtest_main"
        ""
        "tests/SymbolVisitors/SymbolResolverVisitorTest.cpp;tests/SymbolVisitors/SymbolResolverVisitorTestFixture.h;tests/SymbolVisitors/SymbolResolverVisitorTestFixture.cpp"
)

EzPacker_AddTest(
        "TypeCheckVisitorTest"
        "EzSemantics;EzLexer;gtest_main"
        ""
        "tests/SymbolVisitors/TypeCheckVisitorTest.cpp;tests/SymbolVisitors/TypeCheckVisitorTestFixture.h;tests/SymbolVisitors/TypeCheckVisitorTestFixture.cpp"
)

EzPacker_AddTest(
        "AstLowererVisitorTest"
        "EzSemantics;EzLexer;EzMir;gtest_main"
        ""
        "tests/AstLowererVisitor/AstLowererVisitorTest.cpp;tests/AstLowererVisitor/AstLowererVisitorTestFixture.h;tests/AstLowererVisitor/AstLowererVisitorTestFixture.cpp"
)

# ─────────────────────────────────────────────────────────────────────────────
#  LoweringPipeline – file-based integration tests
# ─────────────────────────────────────────────────────────────────────────────

set(LP_FIXTURE
        "tests/LoweringPipeline/LoweringPipelineTestFixture.h;tests/LoweringPipeline/LoweringPipelineTestFixture.cpp"
)

EzPacker_AddTest(
        "LP_ArithmeticLowering"
        "EzSemantics;EzLexer;EzMir;gtest_main"
        ""
        "tests/LoweringPipeline/ArithmeticLoweringTest.cpp;${LP_FIXTURE}"
)

EzPacker_AddTest(
        "LP_BitwiseLowering"
        "EzSemantics;EzLexer;EzMir;gtest_main"
        ""
        "tests/LoweringPipeline/BitwiseLoweringTest.cpp;${LP_FIXTURE}"
)

EzPacker_AddTest(
        "LP_DataMovementLowering"
        "EzSemantics;EzLexer;EzMir;gtest_main"
        ""
        "tests/LoweringPipeline/DataMovementLoweringTest.cpp;${LP_FIXTURE}"
)

EzPacker_AddTest(
        "LP_IfLowering"
        "EzSemantics;EzLexer;EzMir;gtest_main"
        ""
        "tests/LoweringPipeline/IfLoweringTest.cpp;${LP_FIXTURE}"
)

EzPacker_AddTest(
        "LP_WhileLowering"
        "EzSemantics;EzLexer;EzMir;gtest_main"
        ""
        "tests/LoweringPipeline/WhileLoweringTest.cpp;${LP_FIXTURE}"
)

EzPacker_AddTest(
        "LP_LabelLowering"
        "EzSemantics;EzLexer;EzMir;gtest_main"
        ""
        "tests/LoweringPipeline/LabelLoweringTest.cpp;${LP_FIXTURE}"
)

EzPacker_AddTest(
        "LP_ModuleLowering"
        "EzSemantics;EzLexer;EzMir;gtest_main"
        ""
        "tests/LoweringPipeline/ModuleLoweringTest.cpp;${LP_FIXTURE}"
)

EzPacker_AddTest(
        "LP_VariableLowering"
        "EzSemantics;EzLexer;EzMir;gtest_main"
        ""
        "tests/LoweringPipeline/VariableLoweringTest.cpp;${LP_FIXTURE}"
)

EzPacker_AddTest(
        "LP_ImmediateLowering"
        "EzSemantics;EzLexer;EzMir;gtest_main"
        ""
        "tests/LoweringPipeline/ImmediateLoweringTest.cpp;${LP_FIXTURE}"
)

EzPacker_AddTest(
        "LP_ComplexProgramLowering"
        "EzSemantics;EzLexer;EzMir;gtest_main"
        ""
        "tests/LoweringPipeline/ComplexProgramLoweringTest.cpp;${LP_FIXTURE}"
)

