function(GetLibraryBuildType out)
    if (EZPACKER_BUILD_STATIC)
        set(${out} "STATIC" PARENT_SCOPE)
    else ()
        set(${out} "SHARED" PARENT_SCOPE)
    endif ()
endfunction()

function(GetGrammarFiles out)
    set(${out}
            "include/grammar/Grammar.h"
            "include/grammar/Instruction.h"
            "include/grammar/Variable.h"
            "include/grammar/Addressing.h"
            "include/grammar/Combinators.h"
            "include/grammar/Identifier.h"
            "include/grammar/Keyword.h"
            "include/grammar/Module.h"
            "include/grammar/Number.h"
            "include/grammar/Register.h"
            "include/grammar/String.h"
            "include/grammar/TokenType.h"
            "include/grammar/Type.h"

            "src/grammar/Instruction.cpp"
            "src/grammar/Variable.cpp"
            "src/grammar/Addressing.cpp"
            "src/grammar/Combinators.cpp"
            "src/grammar/Identifier.cpp"
            "src/grammar/Keyword.cpp"
            "src/grammar/Module.cpp"
            "src/grammar/Number.cpp"
            "src/grammar/Register.cpp"
            "src/grammar/String.cpp"
            "src/grammar/TokenType.cpp"
            "src/grammar/Type.cpp"
            PARENT_SCOPE)
endfunction()

function(BuildEzFrontend)
    GetLibraryBuildType(EZFRONTEND_BUILD_TYPE)
    message(STATUS "Building EzFrontend as ${EZFRONTEND_BUILD_TYPE}")

    GetGrammarFiles(EZFRONTEND_GRAMMAR_FILES)
    message(STATUS "Adding grammar files: ${EZFRONTEND_GRAMMAR_FILES}")

    add_library("EzFrontend" ${EZFRONTEND_BUILD_TYPE}
            ${EZFRONTEND_GRAMMAR_FILES}
            "include/EzFrontend.h"
            "include/EzFrontendCommon.h"
            "include/BasicParser.h"
            "include/IRTypes.h"
            "include/IParser.h"
            "include/ParseRule.h"
            "include/ast/Ast.h"
            "include/ast/IdentifierNode.h"
            "include/ast/MemoryNode.h"
            "include/ast/TokenTypeNode.h"
            "include/ast/ValueNode.h"
            "include/number/BigInt.h"
            "include/number/IBigNumber.h"
            "include/tokenizer/BasicTokenizer.h"
            "include/tokenizer/ITokenizer.h"

            "src/BasicParser.cpp"
            "src/IRTypes.cpp"
            "src/ParseRule.cpp"
            "src/ast/Ast.cpp"
            "src/ast/IdentifierNode.cpp"
            "src/ast/MemoryNode.cpp"
            "src/ast/TokenTypeNode.cpp"
            "src/ast/ValueNode.cpp"
            "src/number/BigInt.cpp"
            "src/tokenizer/BasicTokenizer.cpp"
    )

    # Set precompiled header.
    target_precompile_headers(EzFrontend PUBLIC "include/EzFrontendCommon.h")

    # Adds EzFrontend's own directory to include path and link GTest and EzLogger.
    target_link_libraries(EzFrontend PUBLIC EzLogger gtest gtest_main)

    message("Including GTest from ${gtest_build_include_dirs}")

    target_include_directories(EzFrontend PUBLIC "include" ${gtest_build_include_dirs})
endfunction()

BuildEzFrontend()

# Add tests
if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzFrontend")

    add_custom_target(CopyTestFiles ALL
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/testData"
            "$<TARGET_FILE_DIR:EzFrontend>/testData"
            COMMENT "Copying test data"
    )

    EzPacker_AddTest(
            "T_InstructionParser"
            ""
            "EzFrontend"
            ""
            "tests/TestInstructionParser.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

    EzPacker_AddTest(
            "T_InstructionParserFile"
            ""
            "EzFrontend"
            "CopyTestFiles"
            "tests/TestInstructionParserFile.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

    EzPacker_AddTest(
            "T_ModuleParser"
            ""
            "EzFrontend"
            ""
            "tests/TestModuleParser.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

    EzPacker_AddTest(
            "T_ModuleParserFile"
            ""
            "EzFrontend"
            "CopyTestFiles"
            "tests/TestModuleParserFile.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

    EzPacker_AddTest(
            "T_NumberParser"
            ""
            "EzFrontend"
            ""
            "tests/TestNumberParser.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

    EzPacker_AddTest(
            "T_MemoryParser"
            ""
            "EzFrontend"
            ""
            "tests/TestMemoryParser.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

    EzPacker_AddTest(
            "T_RegisterParser"
            ""
            "EzFrontend"
            ""
            "tests/TestRegisterParser.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

    EzPacker_AddTest(
            "T_BasicTokenizer"
            ""
            "EzFrontend"
            ""
            "tests/TestTokenizer.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

    EzPacker_AddTest(
            "T_TypeParser"
            ""
            "EzFrontend"
            ""
            "tests/TestTypeParser.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

    EzPacker_AddTest(
            "T_VariableParser"
            ""
            "EzFrontend"
            "CopyTestFiles"
            "tests/TestVariableParser.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

    EzPacker_AddTest(
            "T_VariableParserFile"
            ""
            "EzFrontend"
            "CopyTestFiles"
            "tests/TestVariableParserFile.cpp;tests/BasicParserTest.cpp;tests/BasicParserTest.h"
    )

endif ()
