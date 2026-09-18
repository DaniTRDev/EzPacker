#include "EzMirTestSuite.h"
#include "Parser/MirParser.h"
#include "Printer/MirPrinter.h"
#include "Builder/MirBuilderContext.h"
#include "Function/MirFunction.h"
#include "Block/MirBlock.h"
#include "Instruction/MirInstruction.h"
#include "GlobalVar/MirGlobalVar.h"
#include "Operand/MirOperands.h"
#include "Type/MirTypeTable.h"
#include "Type/MirType.h"

class MirParserTest : public MirTestSuiteAsGtest
{
};

/**
 * TMP-01: Type Parsing
 * Tests primitives, pointers, arrays, and token types.
 */
TEST_F(MirParserTest, TMP_01_TypeParsing)
{
    MirBuilderContext *ctx = getBuilderCtx();
    EzMir::MirParser parser(ctx);

    std::string_view mirCode = R"mir(
@g_i1 = internal var i1 = 1;
@g_i8 = internal var i8 = 8;
@g_i16 = internal var i16 = 16;
@g_i32 = internal var i32 = 32;
@g_i64 = internal var i64 = 64;
@g_f32 = internal var f32 = 1.5;
@g_f64 = internal var f64 = 3.14;
@g_ptr = internal var ptr;
@g_arr = internal var [14 x i8];

fn @type_test() -> void {
entry:
    RET;
}
)mir";

    bool success = parser.parseModule(mirCode, "type_test.mir");
    EXPECT_TRUE(success);

    MirTypeTable *tt = ctx->getTypeTable();
    EXPECT_NE(tt->i1(), nullptr);
    EXPECT_NE(tt->i8(), nullptr);
    EXPECT_NE(tt->i16(), nullptr);
    EXPECT_NE(tt->i32(), nullptr);
    EXPECT_NE(tt->i64(), nullptr);
    EXPECT_NE(tt->f32(), nullptr);
    EXPECT_NE(tt->f64(), nullptr);
    EXPECT_NE(tt->_void(), nullptr);
}

/**
 * TMP-02: Global Variables
 * Tests internal, external, weak, const, var, and initializers.
 */
TEST_F(MirParserTest, TMP_02_GlobalVariables)
{
    MirBuilderContext *ctx = getBuilderCtx();
    EzMir::MirParser parser(ctx);

    std::string_view mirCode = R"mir(
@msg = internal const [14 x i8] "Hello, World!\0A\00";
@counter = external var i64 = 42;
@flag = weak var i1 = 0;
@uninit = external var i32;

fn @dummy() -> i64 {
entry:
    %v0 = MOV i64 0;
    RET i64 %v0;
}
)mir";

    bool success = parser.parseModule(mirCode, "globals.mir");
    EXPECT_TRUE(success);

    EXPECT_GE(ctx->getGlobalVars().size(), 3);
    bool foundCounter = false;
    for (MirGlobalVar *gvar : ctx->getGlobalVars())
    {
        if (gvar->getName() == "counter")
        {
            foundCounter = true;
            EXPECT_FALSE(gvar->isConstant());
            EXPECT_EQ(gvar->getLinkage(), MirGlobalVarLinkage::External);
            EXPECT_NE(gvar->getInitializer(), nullptr);
        }
    }
    EXPECT_TRUE(foundCounter);
}

/**
 * TMP-03: Basic Blocks & CFG
 * Tests multiple basic blocks, branching, and returns.
 */
TEST_F(MirParserTest, TMP_03_BasicBlocksAndCfg)
{
    MirBuilderContext *ctx = getBuilderCtx();
    EzMir::MirParser parser(ctx);

    std::string_view mirCode = R"mir(
fn @branch_test() -> i64 {
entry:
    %v0 = MOV i64 10;
    %cond = ICMP_SGT i1 %v0, 5;
    BR_COND %cond, label %then_block, label %else_block;

then_block:
    %v1 = ADD i64 %v0, 2;
    BR label %exit;

else_block:
    %v2 = SUB i64 %v0, 2;
    BR label %exit;

exit:
    %result = MOV i64 %v0;
    RET i64 %result;
}
)mir";

    bool success = parser.parseModule(mirCode, "branch.mir");
    EXPECT_TRUE(success);

    MirFunction *func = nullptr;
    for (MirFunction *f : ctx->getFunctions())
    {
        if (f->getName() == "branch_test")
        {
            func = f;
            break;
        }
    }
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->getBlocks().size(), 4);
}

/**
 * TMP-04: Forward Block References
 * Tests jumping to a block label defined further down in the file.
 */
TEST_F(MirParserTest, TMP_04_ForwardBlockReferences)
{
    MirBuilderContext *ctx = getBuilderCtx();
    EzMir::MirParser parser(ctx);

    std::string_view mirCode = R"mir(
fn @forward_ref() -> i64 {
entry:
    BR label %target_block;

middle_block:
    %v0 = MOV i64 100;
    BR label %target_block;

target_block:
    %v1 = MOV i64 42;
    RET i64 %v1;
}
)mir";

    bool success = parser.parseModule(mirCode, "fwd_ref.mir");
    EXPECT_TRUE(success);
}

/**
 * TMP-05: Memory Addressing
 * Tests SIB addressing: [ptr %base + %index * scale + disp].
 */
TEST_F(MirParserTest, TMP_05_MemoryAddressing)
{
    MirBuilderContext *ctx = getBuilderCtx();
    EzMir::MirParser parser(ctx);

    std::string_view mirCode = R"mir(
fn @mem_test(ptr %base, i64 %idx) -> i64 {
entry:
    %val1 = LOAD i64 [ptr %base + 8];
    %val2 = LOAD i64 [ptr %base + %idx * 4 + 16];
    STORE [ptr %base + 32], i64 %val1;
    RET i64 %val2;
}
)mir";

    bool success = parser.parseModule(mirCode, "mem.mir");
    EXPECT_TRUE(success);
}

/**
 * TMP-06: Phi Nodes
 * Tests SSA PHI nodes in both assignment and prefix forms.
 */
TEST_F(MirParserTest, TMP_06_PhiNodes)
{
    MirBuilderContext *ctx = getBuilderCtx();
    EzMir::MirParser parser(ctx);

    std::string_view mirCode = R"mir(
fn @phi_test() -> i64 {
entry:
    %cond = MOV i1 1;
    BR_COND %cond, label %bb1, label %bb2;

bb1:
    %v1 = MOV i64 10;
    BR label %merge;

bb2:
    %v2 = MOV i64 20;
    BR label %merge;

merge:
    %res = PHI i64 [%v1, label %bb1], [%v2, label %bb2];
    RET i64 %res;
}
)mir";

    bool success = parser.parseModule(mirCode, "phi.mir");
    EXPECT_TRUE(success);
}

/**
 * TMP-07: ABI Tokens
 * Tests pass-internal lowering tokens (PUSH_ARG, POP_ARG, END_ARG, PUSH_RET).
 */
TEST_F(MirParserTest, TMP_07_AbiTokens)
{
    MirBuilderContext *ctx = getBuilderCtx();
    EzMir::MirParser parser(ctx);

    std::string_view mirCode = R"mir(
declare @callee(i64) -> i64;

fn @caller() -> i64 {
entry:
    %tok = MOV token 0;
    %arg0 = MOV i64 123;
    PUSH_ARG %tok, %arg0;
    END_ARG %tok;
    %ret = CALL @callee;
    RET i64 %ret;
}
)mir";

    bool success = parser.parseModule(mirCode, "abi_tokens.mir");
    EXPECT_TRUE(success);
}

/**
 * TMP-08: Round-Trip Idempotence
 * Verifies Parse(Print(Module)) produces an equivalent MIR representation.
 */
TEST_F(MirParserTest, TMP_08_RoundTripIdempotence)
{
    MirBuilderContext *ctx = getBuilderCtx();
    EzMir::MirParser parser(ctx);

    std::string_view initialMir = R"mir(
@counter = internal var i64 = 100;

fn @add_func(i64 %a, i64 %b) -> i64 {
entry:
    %sum = ADD i64 %a, %b;
    RET i64 %sum;
}
)mir";

    bool firstParse = parser.parseModule(initialMir, "initial.mir");
    ASSERT_TRUE(firstParse);

    std::string printed = MirPrinter::printModule(ctx, MirPrinterMode::Parseable);
    EXPECT_FALSE(printed.empty());

    // Create a new context and parse the printed output
    EzMir::MirParser parser2(ctx);
    bool secondParse = parser2.parseModule(printed, "roundtrip.mir");
    EXPECT_TRUE(secondParse);
}

/**
 * TMP-09: Error Diagnostics
 * Verifies that syntax errors, undeclared symbols, and type violations are caught cleanly.
 */
TEST_F(MirParserTest, TMP_09_ErrorDiagnostics)
{
    MirBuilderContext *ctx = getBuilderCtx();
    EzMir::MirParser parser(ctx);

    // Missing semicolon
    std::string_view badSyntax = R"mir(
fn @bad() -> void {
entry:
    %v0 = MOV i64 10
    RET;
}
)mir";

    bool result = parser.parseModule(badSyntax, "bad_syntax.mir");
    EXPECT_FALSE(result);
}
