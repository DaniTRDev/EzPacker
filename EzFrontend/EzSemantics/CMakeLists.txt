set(EZSEMANTICS_FILES_LIST
        "include/EzSemantics.h"
        "include/EzSemanticsCommon.h"
        "include/ISemanticAnalyzerContext.h"
        "include/SemanticVisitor.h"
        "include/ScopeManager/Scope.h"
        "include/ScopeManager/ScopeManager.h"
        "include/SymbolResolverVisitor/Symbol.h"
        "include/SymbolResolverVisitor/SymbolAnnotation.h"
        "include/SymbolResolverVisitor/SymbolResolverVisitor.h"

        "src/SemanticVisitor.cpp"
        "src/ScopeManager/Scope.cpp"
        "src/ScopeManager/ScopeManager.cpp"
        "src/SymbolResolverVisitor/Symbol.cpp"
        "src/SymbolResolverVisitor/SymbolAnnotation.cpp"
        "src/SymbolResolverVisitor/SymbolResolverVisitor.cpp"
)

EzCMK_ConfigureLib(NAME "EzSemantics"
        FILE_LIST ${EZSEMANTICS_FILES_LIST}
        PRECOMPILED_HEADER "include/EzSemanticsCommon.h"
        LINKED_LIBRARIES "EzLexer"
        INCLUDED_DIRS "include")

EzPacker_AddTest(
        "ScopeManagerTest"
        "EzSemantics;gtest_main"
        ""
        "tests/ScopeManager/ScopeManagerTest.cpp;tests/ScopeManager/ScopeManagerTestFixture.h;tests/ScopeManager/ScopeManagerTestFixture.cpp"
)