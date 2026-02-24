set(EZSEMANTICS_FILES_LIST
        "include/BasicSemanticContext.h"
        "include/EzSemantics.h"
        "include/EzSemanticsCommon.h"
        "include/HighLevelMirVisitor.h"
        "include/SemanticVisitor.h"
        "include/HighLevelMir/HighLevelInstructionSet.h"
        "include/HighLevelMir/HighLevelMirBlock.h"
        "include/HighLevelMir/HighLevelMirDefs.h"
        "include/HighLevelMir/HighLevelMirInstruction.h"
        "include/HighLevelMir/HighLevelMirInstructionOperand.h"
        "include/HighLevelMir/HighLevelMirModule.h"
        "include/HighLevelMirVisitors/InstructionSemanticCheckerVisitor.h"
        "include/HighLevelMirVisitors/ModuleInstructionLowererVisitor.h"
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
        "src/HighLevelMirVisitor.cpp"
        "src/SemanticVisitor.cpp"
        "src/HighLevelMir/HighLevelMirBlock.cpp"
        "src/HighLevelMir/HighLevelMirInstruction.cpp"
        "src/HighLevelMir/HighLevelMirInstructionOperand.cpp"
        "src/HighLevelMir/HighLevelMirModule.cpp"
        "src/HighLevelMirVisitors/InstructionSemanticCheckerVisitor.cpp"
        "src/HighLevelMirVisitors/ModuleInstructionLowererVisitor.cpp"
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
        LINKED_LIBRARIES "EzLexer"
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
        "TypeCastVisitorTest"
        "EzSemantics;EzLexer;gtest_main"
        ""
        "tests/SymbolVisitors/TypeCastVisitorTest.cpp;tests/SymbolVisitors/TypeCastVisitorTestFixture.h;tests/SymbolVisitors/TypeCastVisitorTestFixture.cpp"
)

EzPacker_AddTest(
        "InstructionLowererVisitorTest"
        "EzSemantics;EzLexer;gtest_main"
        ""
        "tests/HighLevelMirVisitors/InstructionLowererVisitorTest.cpp;tests/HighLevelMirVisitors/InstructionLowererVisitorTestFixture.h;tests/HighLevelMirVisitors/InstructionLowererVisitorTestFixture.cpp"
)

EzPacker_AddTest(
        "InstructionSemanticCheckerVisitorTest"
        "EzSemantics;EzLexer;gtest_main"
        ""
        "tests/HighLevelMirVisitors/InstructionSemanticCheckerVisitorTest.cpp;tests/HighLevelMirVisitors/InstructionSemanticCheckerVisitorTestFixture.h;tests/HighLevelMirVisitors/InstructionSemanticCheckerVisitorTestFixture.cpp"
)
