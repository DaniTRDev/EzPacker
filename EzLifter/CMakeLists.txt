function(GetLibraryBuildType out)
    if (EZPACKER_BUILD_STATIC)
        set(${out} "STATIC" PARENT_SCOPE)
    else ()
        set(${out} "SHARED" PARENT_SCOPE)
    endif ()
endfunction()

function(FetchLLVM outLibs)
    find_package(LLVM REQUIRED CONFIG)
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
    message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

    llvm_map_components_to_libnames(mappedLibs support core irreader)

    # Pass mapped libs to the parent scope.
    set(${outLibs} ${mappedLibs} PARENT_SCOPE)

    message(STATUS "Fetched LLVM Libraries: ${outLibs}")
    message(STATUS "Fetched LLVM Definitions: ${LLVM_DEFINITIONS}")
    message(STATUS "Fetched LLVM Include Directory: ${LLVM_INCLUDE_DIRS}")

    include_directories(${LLVM_INCLUDE_DIRS})
    add_definitions(${LLVM_DEFINITIONS})
endfunction()

function(BuildEzLifter)
    GetLibraryBuildType(EZLIFTER_BUILD_TYPE)
    message(STATUS "Building EzLifter as ${EZLIFTER_BUILD_TYPE}")

    add_library("EzLifter" ${EZLIFTER_BUILD_TYPE}
            "include/EzLifter.h"
            "include/EzLifterCommon.h"
            "include/Architectures/IArchitecture.h"
            "include/CodeAnalyzer/CodeAnalyzer.h"
            "include/CodeAnalyzer/ICodeAnalyzerModule.h"
            "include/CodeFlowGraph/CodeFlowGraph.h"
            "include/Decoder/IDecoder.h"
            "include/Decoder/IDecoderDefines.h"
            "include/Decoder/IDecodedInstructionParser.h"
            "include/Decoder/IDecodedOperand.h"
            "include/Decoder/ParsedDecodedInstruction.h"
            "include/Decoder/Zydis/ZydisDecodedInstructionParser.h"
            "include/Decoder/Zydis/EzZydisDecodedOperand.h"
            "include/Decoder/Zydis/EzZydisDecoder.h"
            "include/InstructionLifters/IInstructionLifter.h"
            "include/InstructionLifters/InstructionLifters.h"

            "src/EzLifter.cpp"
            "src/Architectures/x64.cpp"
            "src/CodeAnalyzer/CodeAnalyzer.cpp"
            "src/CodeFlowGraph/CodeFlowGraph.cpp"
            "src/Decoder/ParsedDecodedInstruction.cpp"
            "src/Decoder/Zydis/ZydisDecodedInstructionParser.cpp"
            "src/Decoder/Zydis/EzZydisDecodedOperand.cpp"
            "src/Decoder/Zydis/EzZydisDecoder.cpp"
    )

    # Set the LLVM static definition if needed.
    if (EZLIFTER_BUILD_TYPE STREQUAL "STATIC")
        target_compile_definitions(EzLifter PRIVATE LLVM_STATIC)
    endif ()

    # Set precompiled header.
    target_precompile_headers(EzLifter PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/EzLifterCommon.h")

    # Adds EzLifter's own directory to include path and link GTest, Zydis and LLVM.
    target_link_libraries(EzLifter PUBLIC Zydis EzLogger gtest gtest_main ${LLVM_LIBRARIES})
    target_include_directories(EzLifter PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" ${gtest_build_include_dirs})
endfunction()

FetchLLVM(LLVM_LIBRARIES)
BuildEzLifter()

# Add tests
if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzLifter")
    EzPacker_AddTest(
            "T_EzZydisDecodedOperand"
            "-DEZLIFTER"
            "EzLifter"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/TestEzZydisDecodedOperand.cpp"
    )
    EzPacker_AddTest(
            "T_ZydisDecodedInstructionParser"
            "-DEZLIFTER"
            "EzLifter"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/TestZydisDecodedInstructionParser.cpp"
    )
    EzPacker_AddTest("T_ZydisDecoder"
            "-DEZLIFTER"
            "EzLifter"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/TestZydisDecoder.cpp"
    )
endif ()
