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
        "include/AstLowererVisitor/GenericAstLowerer.h"
        "include/AstLowererVisitor/IfLowerer.h"
        "include/AstLowererVisitor/ImmediateLowerer.h"
        "include/AstLowererVisitor/InstructionLowerer.h"
        "include/AstLowererVisitor/LabelLowerer.h"
        "include/AstLowererVisitor/LoweringContext.h"
        "include/AstLowererVisitor/MemoryLowerer.h"
        "include/AstLowererVisitor/ModuleLowerer.h"
        "include/AstLowererVisitor/SwitchLowerer.h"
        "include/AstLowererVisitor/TypeLowerer.h"
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
        "src/AstLowererVisitor/TypeLowerer.cpp"
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

if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzSemantics")

    EzPacker_AddTest(
            "T_ScopeAndSymbol"
            "EzSemantics"
            ""
            "tests/T_ScopeAndSymbol.cpp"
    )

    EzPacker_AddTest(
            "T_SymbolDefinitionVisitor"
            "EzSemantics"
            ""
            "tests/T_SymbolDefinitionVisitor.cpp"
    )

    EzPacker_AddTest(
            "T_SemanticPipeline"
            "EzSemantics"
            ""
            "tests/T_SemanticPipeline.cpp"
    )

    EzPacker_AddTest(
            "T_AstLowerer"
            "EzSemantics"
            ""
            "tests/T_AstLowerer.cpp"
    )
endif ()
