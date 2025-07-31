function(GetLibraryBuildType out)
    if (EZPACKER_BUILD_STATIC)
        set(${out} "STATIC" PARENT_SCOPE)
    else ()
        set(${out} "SHARED" PARENT_SCOPE)
    endif ()
endfunction()

function(BuildEzLexer)
    GetLibraryBuildType(EZLEXER_BUILD_TYPE)
    message(STATUS "Building EzLexer as ${EZLEXER_BUILD_TYPE}")

    add_library("EzLexer" ${EZLEXER_BUILD_TYPE}
            "include/Parser/BasicParsingContext.h"
            "include/EzLexer.h"
            "include/EzLexerCommon.h"
            "include/AstNode/AstNode.h"
            "include/AstNode/AstNodes.h"
            "include/ErrorCollector/ErrorCollector.h"
            "include/Parser/ComplexParsers.h"
            "include/Parser/IParsingContext.h"
            "include/Parser/MemoryParsers.h"
            "include/Parser/Parsers.h"
            "include/Parser/PrimitiveParsers.h"
            "include/Parser/TokenParsers.h"
            "include/Parser/Rule.h"
            "include/Parser/Rules.h"
            "include/Tokenizer/BasicTokenizer.h"
            "include/Tokenizer/ITokenizer.h"

            "src/AstNode/AstNode.cpp"
            "src/AstNode/AstNodes.cpp"
            "src/ErrorCollector/ErrorCollector.cpp"
            "src/Parser/BasicParsingContext.cpp"
            "src/Parser/ComplexParsers.cpp"
            "src/Parser/MemoryParsers.cpp"
            "src/Parser/PrimitiveParsers.cpp"
            "src/Parser/TokenParsers.cpp"
            "src/Parser/Rule.cpp"
            "src/Parser/Rules.cpp"
            "src/Tokenizer/BasicTokenizer.cpp"
    )

    # Set precompiled header.
    target_precompile_headers(EzLexer PUBLIC "include/EzLexerCommon.h")

    # Adds EzLexer's own directory to include path and link GTest and EzLogger.
    target_link_libraries(EzLexer PUBLIC EzCore EzLogger)
    target_include_directories(EzLexer PUBLIC "include" ${gtest_build_include_dirs})
    set_target_properties(EzLexer PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endfunction()

BuildEzLexer()

# Add tests
if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzLexer")

    EzPacker_AddTest(
            "T_InstructionParser"
            ""
            "EzLexer"
            ""
            "tests/TestInstructionParser.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

    EzPacker_AddTest(
            "T_InstructionParserFile"
            ""
            "EzLexer"
            ""
            "tests/TestInstructionParserFile.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

    EzPacker_AddTest(
            "T_ModuleParser"
            ""
            "EzLexer"
            ""
            "tests/TestModuleParser.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

    EzPacker_AddTest(
            "T_ModuleWithLabelsParser"
            ""
            "EzLexer"
            ""
            "tests/TestModuleWithLabelsParser.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

    EzPacker_AddTest(
            "T_ModuleParserFile"
            ""
            "EzLexer"
            ""
            "tests/TestModuleParserFile.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

    EzPacker_AddTest(
            "T_NumberParser"
            ""
            "EzLexer"
            ""
            "tests/TestNumberParser.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

    EzPacker_AddTest(
            "T_MemoryParser"
            ""
            "EzLexer"
            ""
            "tests/TestMemoryParser.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

    EzPacker_AddTest(
            "T_VirtualVariableParser"
            ""
            "EzLexer"
            ""
            "tests/TestVirtualVariableParser.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

    EzPacker_AddTest(
            "T_BasicTokenizer"
            ""
            "EzLexer"
            ""
            "tests/TestTokenizer.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

    EzPacker_AddTest(
            "T_TypeParser"
            ""
            "EzLexer"
            ""
            "tests/TestTypeParser.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

    EzPacker_AddTest(
            "T_VariableParser"
            ""
            "EzLexer"
            ""
            "tests/TestVariableParser.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

    EzPacker_AddTest(
            "T_VariableParserFile"
            ""
            "EzLexer"
            ""
            "tests/TestVariableParserFile.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

endif ()
