function(GetLibraryBuildType out)
    if (EZPACKER_BUILD_STATIC)
        set(${out} "STATIC" PARENT_SCOPE)
    else ()
        set(${out} "SHARED" PARENT_SCOPE)
    endif ()
endfunction()

function(GetParserFiles out)
    set(${out}
            "include/Parser/Grammar/Grammar.h"
            "include/Parser/Grammar/Instruction.h"
            "include/Parser/Grammar/Addressing.h"
            "include/Parser/Grammar/Combinators.h"
            "include/Parser/Grammar/Identifier.h"
            "include/Parser/Grammar/Keyword.h"
            "include/Parser/Grammar/Module.h"
            "include/Parser/Grammar/Number.h"
            "include/Parser/Grammar/String.h"
            "include/Parser/Grammar/TokenType.h"
            "include/Parser/Grammar/Type.h"
            "include/Parser/Grammar/Variable.h"
            "include/Parser/Grammar/VirtualVariable.h"
            "include/Parser/Ast/Ast.h"
            "include/Parser/Ast/IdentifierNode.h"
            "include/Parser/Ast/MemoryNode.h"
            "include/Parser/Ast/TokenTypeNode.h"
            "include/Parser/Ast/ValueNode.h"

            "src/Parser/Ast/Ast.cpp"
            "src/Parser/Ast/IdentifierNode.cpp"
            "src/Parser/Ast/MemoryNode.cpp"
            "src/Parser/Ast/TokenTypeNode.cpp"
            "src/Parser/Ast/ValueNode.cpp"
            "src/Parser/Grammar/Instruction.cpp"
            "src/Parser/Grammar/Addressing.cpp"
            "src/Parser/Grammar/Combinators.cpp"
            "src/Parser/Grammar/Identifier.cpp"
            "src/Parser/Grammar/Keyword.cpp"
            "src/Parser/Grammar/Module.cpp"
            "src/Parser/Grammar/Number.cpp"
            "src/Parser/Grammar/String.cpp"
            "src/Parser/Grammar/TokenType.cpp"
            "src/Parser/Grammar/Type.cpp"
            "src/Parser/Grammar/Variable.cpp"
            "src/Parser/Grammar/VirtualVariableNode.cpp"
            PARENT_SCOPE)
endfunction()

function(GetNormalizerFiles out)
    set(${out}
            "include/Semantics/IAstVisitor.h"
            "include/Semantics/NormalTable.h"
            "include/Semantics/Normalizer/INormalizer.h"
            "include/Semantics/Normalizer/InstructionNormalizer.h"
            "include/Semantics/Normalizer/MemoryNodeNormalizer.h"
            "include/Semantics/Normalizer/ModuleNormalizer.h"
            "include/Semantics/Normalizer/OperandNormalizer.h"
            "include/Semantics/Normalizer/SymbolNormalizer.h"
            "include/Semantics/Normalizer/TypeNormalizer.h"
            "include/Semantics/Normalizer/ValueNormalizer.h"
            "include/Semantics/Normalizer/VirtualVariableNormalizer.h"
            "include/Semantics/Tables/SemanticTable.h"
            "include/Semantics/Tables/SymbolTable.h"
            "include/Semantics/Tables/TypeTable.h"

            "src/Semantics/NormalTable.cpp"
            "src/Semantics/Normalizer/InstructionNormalizer.cpp"
            "src/Semantics/Normalizer/MemoryNodeNormalizer.cpp"
            "src/Semantics/Normalizer/ModuleNormalizer.cpp"
            "src/Semantics/Normalizer/OperandNormalizer.cpp"
            "src/Semantics/Normalizer/SymbolNormalizer.cpp"
            "src/Semantics/Normalizer/TypeNormalizer.cpp"
            "src/Semantics/Normalizer/ValueNormalizer.cpp"
            "src/Semantics/Normalizer/VirtualVariableNodeNormalizer.cpp"
            "src/Semantics/Tables/SymbolTable.cpp"
            "src/Semantics/Tables/TypeTable.cpp"
            PARENT_SCOPE)
endfunction()

function(BuildEzFrontend)
    GetLibraryBuildType(EZFRONTEND_BUILD_TYPE)
    message(STATUS "Building EzFrontend as ${EZFRONTEND_BUILD_TYPE}")

    GetParserFiles(EZFRONTEND_PARSER_FILES)
    message(STATUS "Adding parser files: ${EZFRONTEND_PARSER_FILES}")

    GetNormalizerFiles(EZFRONTEND_NORMALIZER_FILES)
    message(STATUS "Adding normalizer files: ${EZFRONTEND_NORMALIZER_FILES}")

    add_library("EzFrontend" ${EZFRONTEND_BUILD_TYPE}
            ${EZFRONTEND_PARSER_FILES}
            ${EZFRONTEND_NORMALIZER_FILES}
            "include/EzFrontend.h"
            "include/EzFrontendCommon.h"
            "include/IFrontendLogSink.h"
            "include/Parser/BasicParser.h"
            "include/IRTypes.h"
            "include/Parser/IParser.h"
            "include/Parser/ParseRule.h"

            "include/Number/BigInt.h"
            "include/Number/IBigNumber.h"
            "include/Tokenizer/BasicTokenizer.h"
            "include/Tokenizer/ITokenizer.h"

            "src/Parser/BasicParser.cpp"
            "src/IRTypes.cpp"
            "src/Parser/ParseRule.cpp"
            "src/Number/BigInt.cpp"
            "src/Tokenizer/BasicTokenizer.cpp"
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
