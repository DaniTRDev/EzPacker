
set(EZLEXER_FILES_LIST
        "include/EzLexer.h"
        "include/EzLexerCommon.h"
        "include/AstNode/AstNode.h"
        "include/AstNodes/ImmediateOperand.h"
        "include/AstNodes/Instruction.h"
        "include/AstNodes/Label.h"
        "include/AstNodes/MemoryOperand.h"
        "include/AstNodes/Module.h"
        "include/AstNodes/Variable.h"
        "include/ErrorCollector/ErrorCollector.h"
        "include/AstNodeParser/AstNodeParsingUtils.h"
        "include/AstNodeParser/IAstNodeParser.h"
        "include/AstNodeParser/IParsingContext.h"
        "include/AstNodeParser/SingleThreadParsingContext.h"
        "include/AstNodeParsers/ImmediateOperandParser.h"
        "include/AstNodeParsers/InstructionParser.h"
        "include/AstNodeParsers/LabelParser.h"
        "include/AstNodeParsers/MemoryOperandParser.h"
        "include/AstNodeParsers/ModuleParser.h"
        "include/AstNodeParsers/VariableParser.h"
        "include/Tokenizer/BasicTokenizer.h"
        "include/Tokenizer/ITokenizer.h"

        "src/AstNode/AstNode.cpp"
        "src/AstNodes/ImmediateOperand.cpp"
        "src/AstNodes/Instruction.cpp"
        "src/AstNodes/Label.cpp"
        "src/AstNodes/MemoryOperand.cpp"
        "src/AstNodes/Module.cpp"
        "src/AstNodes/Variable.cpp"
        "src/AstNodeParser/AstNodeParsingUtils.cpp"
        "src/AstNodeParser/SingleThreadParsingContext.cpp"
        "src/AstNodeParsers/ImmediateOperandParser.cpp"
        "src/AstNodeParsers/InstructionParser.cpp"
        "src/AstNodeParsers/LabelParser.cpp"
        "src/AstNodeParsers/MemoryOperandParser.cpp"
        "src/AstNodeParsers/ModuleParser.cpp"
        "src/AstNodeParsers/VariableParser.cpp"
        "src/ErrorCollector/ErrorCollector.cpp"
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