# EzAnnotator CMake build configuration

function(GetLibraryBuildType out)
    if (EZPACKER_BUILD_STATIC)
        set(${out} "STATIC" PARENT_SCOPE)
    else ()
        set(${out} "SHARED" PARENT_SCOPE)
    endif ()
endfunction()

function(BuildEzAnnotator)
    GetLibraryBuildType(EZANNOTATOR_BUILD_TYPE)
    message(STATUS "Building EzAnnotator as ${EZANNOTATOR_BUILD_TYPE}")

    add_library("EzAnnotator" ${EZANNOTATOR_BUILD_TYPE}
            "include/EzAnnotator.h"
            "include/EzAnnotatorCommon.h"
            "include/InstructionTable.h"
            "include/Annotations/ConstantAnnotation.h"
            "include/Annotations/InstructionAnnotation.h"
            "include/Annotations/LabelAnnotation.h"
            "include/Annotations/MemoryRefAnnotation.h"
            "include/Annotations/ModuleAnnotation.h"
            "include/Annotations/SymbolAbleAnnotation.h"
            "include/Annotations/SymbolAnnotation.h"
            "include/Annotations/TypeAbleAnnotation.h"
            "include/Annotations/TypeAnnotation.h"
            "include/Annotations/VariableAnnotation.h"
            "include/NodeAnnotators/ConstantAnnotator.h"
            "include/NodeAnnotators/INodeAnnotator.h"
            "include/NodeAnnotators/InstructionAnnotator.h"
            "include/NodeAnnotators/LabelAnnotator.h"
            "include/NodeAnnotators/MemoryRefAnnotator.h"
            "include/NodeAnnotators/ModuleAnnotator.h"
            "include/NodeAnnotators/ScopeAbleAnnotator.h"
            "include/NodeAnnotators/SymbolAnnotator.h"
            "include/NodeAnnotators/TypeAnnotator.h"
            "include/NodeAnnotators/VariableAnnotator.h"
            "include/NodeAnnotators/VirtualVariableAnnotator.h"
            "include/ScopedTable/ScopedSymbol.h"
            "include/ScopedTable/ScopedTable.h"
            "include/ScopedTable/ScopedType.h"

            "src/Annotations/ConstantAnnotation.cpp"
            "src/Annotations/InstructionAnnotation.cpp"
            "src/Annotations/LabelAnnotation.cpp"
            "src/Annotations/MemoryRefAnnotation.cpp"
            "src/Annotations/ModuleAnnotation.cpp"
            "src/Annotations/SymbolAbleAnnotation.cpp"
            "src/Annotations/SymbolAnnotation.cpp"
            "src/Annotations/TypeAbleAnnotation.cpp"
            "src/Annotations/TypeAnnotation.cpp"
            "src/Annotations/VariableAnnotation.cpp"
            "src/NodeAnnotators/ConstantAnnotator.cpp"
            "src/NodeAnnotators/InstructionAnnotator.cpp"
            "src/NodeAnnotators/LabelAnnotator.cpp"
            "src/NodeAnnotators/MemoryRefAnnotator.cpp"
            "src/NodeAnnotators/ModuleAnnotator.cpp"
            "src/NodeAnnotators/ScopeAbleAnnotator.cpp"
            "src/NodeAnnotators/SymbolAnnotator.cpp"
            "src/NodeAnnotators/TypeAnnotator.cpp"
            "src/NodeAnnotators/VariableAnnotator.cpp"
            "src/NodeAnnotators/VirtualVariableAnnotator.cpp"
            "src/ScopedTable/ScopedSymbol.cpp"
            "src/ScopedTable/ScopedType.cpp"
    )

    # Set precompiled header
    target_precompile_headers(EzAnnotator PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/EzAnnotatorCommon.h")

    # Add include directories and link dependencies
    target_link_libraries(EzAnnotator PUBLIC libtommath EzLexer)
    target_include_directories(EzAnnotator PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    set_target_properties(EzAnnotator PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endfunction()

BuildEzAnnotator()

# Add tests
if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzAnnotator")

    EzPacker_AddTest(
            "T_ScopedSymbol"
            ""
            "EzAnnotator"
            ""
            "tests/TestScopedSymbol.cpp;tests/BasicAnnotatorTest.cpp;tests/BasicAnnotatorTest.h"
    )

    EzPacker_AddTest(
            "T_VirtualVariableAnnotator"
            ""
            "EzAnnotator"
            ""
            "tests/TestVirtualVariable.cpp;tests/BasicAnnotatorTest.cpp;tests/BasicAnnotatorTest.h"
    )

    EzPacker_AddTest(
            "T_ConstantAnnotator"
            ""
            "EzAnnotator"
            ""
            "tests/TestConstant.cpp;tests/BasicAnnotatorTest.cpp;tests/BasicAnnotatorTest.h"
    )

    EzPacker_AddTest(
            "T_TypeAnnotator"
            ""
            "EzAnnotator"
            ""
            "tests/TestType.cpp;tests/BasicAnnotatorTest.cpp;tests/BasicAnnotatorTest.h"
    )

    EzPacker_AddTest(
            "T_MemoryRefAnnotator"
            ""
            "EzAnnotator"
            ""
            "tests/TestMemoryRef.cpp;tests/BasicAnnotatorTest.cpp;tests/BasicAnnotatorTest.h"
    )

    EzPacker_AddTest(
            "T_InstructionAnnotator"
            ""
            "EzAnnotator"
            ""
            "tests/TestInstruction.cpp;tests/BasicAnnotatorTest.cpp;tests/BasicAnnotatorTest.h"
    )

    EzPacker_AddTest(
            "T_ModuleHeaderAnnotator"
            ""
            "EzAnnotator"
            ""
            "tests/TestModuleHeader.cpp;tests/BasicAnnotatorTest.cpp;tests/BasicAnnotatorTest.h"
    )

    EzPacker_AddTest(
            "T_ModuleBodyAnnotator"
            ""
            "EzAnnotator"
            ""
            "tests/TestModuleBody.cpp;tests/BasicAnnotatorTest.cpp;tests/BasicAnnotatorTest.h"
    )

    EzPacker_AddTest(
            "T_VariableAnnotator"
            ""
            "EzAnnotator"
            ""
            "tests/TestVariable.cpp;tests/BasicAnnotatorTest.cpp;tests/BasicAnnotatorTest.h"
    )
endif ()
