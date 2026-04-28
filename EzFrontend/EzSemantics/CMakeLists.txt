set(EZSEMANTICS_FILES_LIST
        "include/BasicSemanticContext.h"
        "include/EzSemantics.h"
        "include/EzSemanticsCommon.h"
        "include/SemanticVisitor.h"
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
endif ()
