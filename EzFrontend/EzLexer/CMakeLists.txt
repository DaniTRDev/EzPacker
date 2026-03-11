
set(EZLEXER_FILES_LIST
        "include/EzLexer.h"
        "include/EzLexerCommon.h"
        "include/AstNode/AstNode.h"
        "include/AstNode/AstNodeContainer.h"
        "include/AstNode/AstNodeVisitor.h"
        "include/AstNodes/BreakAstNode.h"
        "include/AstNodes/CodeScope.h"
        "include/AstNodes/ConditionAstNode.h"
        "include/AstNodes/ContinueAstNode.h"
        "include/AstNodes/ForAstNode.h"
        "include/AstNodes/IfAstNode.h"
        "include/AstNodes/ImmediateOperand.h"
        "include/AstNodes/IncludeAstNode.h"
        "include/AstNodes/Instruction.h"
        "include/AstNodes/Label.h"
        "include/AstNodes/MemoryOperand.h"
        "include/AstNodes/Module.h"
        "include/AstNodes/SwitchAstNode.h"
        "include/AstNodes/SwitchCaseAstNode.h"
        "include/AstNodes/Variable.h"
        "include/AstNodes/WhileAstNode.h"
        "include/AstNodeParsers/BasicParsingContext.h"
        "include/AstNodeParsers/IAstNodeParser.h"
        "include/AstNodeParsers/ParserBatch.h"
        "include/AstNodeParsers/Parsers/BreakParser.h"
        "include/AstNodeParsers/Parsers/CodeScopeParser.h"
        "include/AstNodeParsers/Parsers/ConditionParser.h"
        "include/AstNodeParsers/Parsers/ContinueParser.h"
        "include/AstNodeParsers/Parsers/ForParser.h"
        "include/AstNodeParsers/Parsers/IfParser.h"
        "include/AstNodeParsers/Parsers/ImmediateParser.h"
        "include/AstNodeParsers/Parsers/IncludeParser.h"
        "include/AstNodeParsers/Parsers/InstructionParser.h"
        "include/AstNodeParsers/Parsers/LabelParser.h"
        "include/AstNodeParsers/Parsers/MemoryOperandParser.h"
        "include/AstNodeParsers/Parsers/ModuleParser.h"
        "include/AstNodeParsers/Parsers/VariableParser.h"
        "include/AstNodeParsers/Parsers/SwitchParser.h"
        "include/AstNodeParsers/Parsers/WhileParser.h"
        "include/Tokenizer/BasicTokenizer.h"

        "src/AstNode/AstNode.cpp"
        "src/AstNode/AstNodeContainer.cpp"
        "src/AstNode/AstNodeVisitor.cpp"
        "src/AstNodes/BreakAstNode.cpp"
        "src/AstNodes/CodeScope.cpp"
        "src/AstNodes/ConditionAstNode.cpp"
        "src/AstNodes/ContinueAstNode.cpp"
        "src/AstNodes/ForAstNode.cpp"
        "src/AstNodes/IfAstNode.cpp"
        "src/AstNodes/ImmediateOperand.cpp"
        "src/AstNodes/IncludeAstNode.cpp"
        "src/AstNodes/Instruction.cpp"
        "src/AstNodes/Label.cpp"
        "src/AstNodes/MemoryOperand.cpp"
        "src/AstNodes/Module.cpp"
        "src/AstNodes/SwitchAstNode.cpp"
        "src/AstNodes/SwitchCaseAstNode.cpp"
        "src/AstNodes/Variable.cpp"
        "src/AstNodes/WhileAstNode.cpp"
        "src/AstNodeParsers/BasicParsingContext.cpp"
        "src/AstNodeParsers/ParserBatch.cpp"
        "src/AstNodeParsers/Parsers/BreakParser.cpp"
        "src/AstNodeParsers/Parsers/CodeScopeParser.cpp"
        "src/AstNodeParsers/Parsers/ConditionParser.cpp"
        "src/AstNodeParsers/Parsers/ContinueParser.cpp"
        "src/AstNodeParsers/Parsers/ForParser.cpp"
        "src/AstNodeParsers/Parsers/IfParser.cpp"
        "src/AstNodeParsers/Parsers/ImmediateParser.cpp"
        "src/AstNodeParsers/Parsers/IncludeParser.cpp"
        "src/AstNodeParsers/Parsers/InstructionParser.cpp"
        "src/AstNodeParsers/Parsers/LabelParser.cpp"
        "src/AstNodeParsers/Parsers/MemoryOperandParser.cpp"
        "src/AstNodeParsers/Parsers/ModuleParser.cpp"
        "src/AstNodeParsers/Parsers/VariableParser.cpp"
        "src/AstNodeParsers/Parsers/SwitchParser.cpp"
        "src/AstNodeParsers/Parsers/WhileParser.cpp"
        "src/Tokenizer/BasicTokenizer.cpp"
)

EzCMK_ConfigureLib(NAME "EzLexer"
        FILE_LIST ${EZLEXER_FILES_LIST}
        PRECOMPILED_HEADER "include/EzLexerCommon.h"
        LINKED_LIBRARIES "EzCore;EzLogger;libtommath"
        INCLUDED_DIRS "include")

if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzLexer")

    EzPacker_AddTest(
            "T_BasicTokenizer"
            "EzLexer"
            ""
            "tests/T_BasicTokenizer.cpp"
    )

    EzPacker_AddTest(
            "T_InstructionParser"
            "EzLexer"
            ""
            "tests/T_InstructionParser.cpp"
    )

    EzPacker_AddTest(
            "T_ModuleParser"
            "EzLexer"
            ""
            "tests/T_ModuleParser.cpp"
    )

    EzPacker_AddTest(
            "T_VariableParser"
            "EzLexer"
            ""
            "tests/T_VariableParser.cpp"
    )

    EzPacker_AddTest(
            "T_MemoryParser"
            "EzLexer"
            ""
            "tests/T_MemoryParser.cpp"
    )

    EzPacker_AddTest(
            "T_ControlFlowParsers"
            "EzLexer"
            ""
            "tests/T_ControlFlowParsers.cpp"
    )

    EzPacker_AddTest(
            "T_ParserBatch"
            "EzLexer"
            ""
            "tests/T_ParserBatch.cpp"
    )
endif ()

