
set(EZLEXER_FILES_LIST
        "include/EzLexer.h"
        "include/EzLexerCommon.h"
        "include/AstNode/AstNode.h"
        "include/AstNode/AstNodeContainer.h"
        "include/AstNode/AstNodeVisitor.h"
        "include/AstNodes/CodeScope.h"
        "include/AstNodes/ConditionAstNode.h"
        "include/AstNodes/IfAstNode.h"
        "include/AstNodes/ImmediateOperand.h"
        "include/AstNodes/Instruction.h"
        "include/AstNodes/Label.h"
        "include/AstNodes/MemoryOperand.h"
        "include/AstNodes/Module.h"
        "include/AstNodes/Variable.h"
        "include/AstNodes/WhileAstNode.h"
        "include/AstNodeParsers/BasicParsingContext.h"
        "include/AstNodeParsers/IAstNodeParser.h"
        "include/AstNodeParsers/ParserBatch.h"
        "include/AstNodeParsers/Parsers/CodeScopeParser.h"
        "include/AstNodeParsers/Parsers/ConditionParser.h"
        "include/AstNodeParsers/Parsers/IfParser.h"
        "include/AstNodeParsers/Parsers/ImmediateParser.h"
        "include/AstNodeParsers/Parsers/InstructionParser.h"
        "include/AstNodeParsers/Parsers/LabelParser.h"
        "include/AstNodeParsers/Parsers/MemoryOperandParser.h"
        "include/AstNodeParsers/Parsers/ModuleParser.h"
        "include/AstNodeParsers/Parsers/VariableParser.h"
        "include/AstNodeParsers/Parsers/WhileParser.h"
        "include/Tokenizer/BasicTokenizer.h"

        "src/AstNode/AstNode.cpp"
        "src/AstNode/AstNodeContainer.cpp"
        "src/AstNode/AstNodeVisitor.cpp"
        "src/AstNodes/CodeScope.cpp"
        "src/AstNodes/ConditionAstNode.cpp"
        "src/AstNodes/IfAstNode.cpp"
        "src/AstNodes/ImmediateOperand.cpp"
        "src/AstNodes/Instruction.cpp"
        "src/AstNodes/Label.cpp"
        "src/AstNodes/MemoryOperand.cpp"
        "src/AstNodes/Module.cpp"
        "src/AstNodes/Variable.cpp"
        "src/AstNodes/WhileAstNode.cpp"
        "src/AstNodeParsers/BasicParsingContext.cpp"
        "src/AstNodeParsers/ParserBatch.cpp"
        "src/AstNodeParsers/Parsers/CodeScopeParser.cpp"
        "src/AstNodeParsers/Parsers/ConditionParser.cpp"
        "src/AstNodeParsers/Parsers/IfParser.cpp"
        "src/AstNodeParsers/Parsers/ImmediateParser.cpp"
        "src/AstNodeParsers/Parsers/InstructionParser.cpp"
        "src/AstNodeParsers/Parsers/LabelParser.cpp"
        "src/AstNodeParsers/Parsers/MemoryOperandParser.cpp"
        "src/AstNodeParsers/Parsers/ModuleParser.cpp"
        "src/AstNodeParsers/Parsers/VariableParser.cpp"
        "src/AstNodeParsers/Parsers/WhileParser.cpp"
        "src/Tokenizer/BasicTokenizer.cpp"
)

EzCMK_ConfigureLib(NAME "EzLexer"
        FILE_LIST ${EZLEXER_FILES_LIST}
        PRECOMPILED_HEADER "include/EzLexerCommon.h"
        LINKED_LIBRARIES "EzCore;EzLogger;libtommath"
        INCLUDED_DIRS "include")

EzPacker_AddTest(
        "TokenizerTest"
        "EzLexer;gtest_main"
        ""
        "tests/Tokenizer/TokenizerTest.cpp;tests/Tokenizer/TokenizerTestFixture.h;tests/Tokenizer/TokenizerTestFixture.cpp"
)

EzPacker_AddTest(
        "ParsersTest_Immediate"
        "EzLexer;gtest_main"
        ""
        "tests/Parsers/ParsersTestImmediate.cpp;tests/Parsers/ParsersTestFixture.h;tests/Parsers/ParsersTestFixture.cpp"
)

EzPacker_AddTest(
        "ParsersTest_Variable"
        "EzLexer;gtest_main"
        ""
        "tests/Parsers/ParsersTestVariable.cpp;tests/Parsers/ParsersTestFixture.h;tests/Parsers/ParsersTestFixture.cpp"
)

EzPacker_AddTest(
        "ParsersTest_Memory"
        "EzLexer;gtest_main"
        ""
        "tests/Parsers/ParsersTestMemory.cpp;tests/Parsers/ParsersTestFixture.h;tests/Parsers/ParsersTestFixture.cpp"
)

EzPacker_AddTest(
        "ParsersTest_Instruction"
        "EzLexer;gtest_main"
        ""
        "tests/Parsers/ParsersTestInstruction.cpp;tests/Parsers/ParsersTestFixture.h;tests/Parsers/ParsersTestFixture.cpp"
)

EzPacker_AddTest(
        "ParsersTest_Label"
        "EzLexer;gtest_main"
        ""
        "tests/Parsers/ParsersTestLabel.cpp;tests/Parsers/ParsersTestFixture.h;tests/Parsers/ParsersTestFixture.cpp"
)

EzPacker_AddTest(
        "ParsersTest_Module"
        "EzLexer;gtest_main"
        ""
        "tests/Parsers/ParsersTestModule.cpp;tests/Parsers/ParsersTestFixture.h;tests/Parsers/ParsersTestFixture.cpp"
)

EzPacker_AddTest(
        "ParsersTest_Condition"
        "EzLexer;gtest_main"
        ""
        "tests/Parsers/ParsersTestCondition.cpp;tests/Parsers/ParsersTestFixture.h;tests/Parsers/ParsersTestFixture.cpp"
)

EzPacker_AddTest(
        "ParsersTest_If"
        "EzLexer;gtest_main"
        ""
        "tests/Parsers/ParsersTestIf.cpp;tests/Parsers/ParsersTestFixture.h;tests/Parsers/ParsersTestFixture.cpp"
)

EzPacker_AddTest(
        "ParsersTest_While"
        "EzLexer;gtest_main"
        ""
        "tests/Parsers/ParsersTestWhile.cpp;tests/Parsers/ParsersTestFixture.h;tests/Parsers/ParsersTestFixture.cpp"
)
