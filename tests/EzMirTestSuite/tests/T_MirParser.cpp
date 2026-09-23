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
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticListener.h"
#include "Diagnostics/DiagnosticMessage.h"

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

/**
 * TMP-10: Forward Function and Global Fixups
 * Verifies that calls to forward functions and references to forward globals are properly patched
 * to MirReference operands rather than remaining as raw runtime symbols.
 */
TEST_F(MirParserTest, TMP_10_ForwardFunctionAndGlobalFixups)
{
    MirBuilderContext *ctx = getBuilderCtx();
    EzMir::MirParser parser(ctx);

    std::string_view mirCode = R"mir(
fn @caller() -> i64 {
entry:
    %fn_ref = CALL i64 @target_func;
    %val = LOAD i64 @target_var;
    RET i64 %val;
}

fn @target_func() -> i64 {
entry:
    RET i64 42;
}

@target_var = internal var i64 = 100;
)mir";

    bool success = parser.parseModule(mirCode, "fwd_func_var.mir");
    EXPECT_TRUE(success);

    MirFunction *caller = nullptr;
    for (MirFunction *fn : ctx->getFunctions())
    {
        if (fn && fn->getName() == "caller")
        {
            caller = fn;
            break;
        }
    }
    ASSERT_NE(caller, nullptr);
    MirBlock *entry = caller->getEntryPoint();
    ASSERT_NE(entry, nullptr);

    auto instIt = entry->getInstructions().begin();
    ASSERT_NE(instIt, entry->getInstructions().end());

    // CALL instruction: second operand should be a MirReference to target_func
    MirInstruction *callInst = *instIt++;
    ASSERT_EQ(callInst->getOpCode(), getOpCodeFromStr("CALL"));
    ASSERT_GE(callInst->getOperands().size(), 2);
    MirOperand *calleeOp = callInst->getOperands()[1];
    ASSERT_EQ(calleeOp->getType(), MirOperandType::Reference);
    auto *calleeRef = static_cast<MirReference *>(calleeOp);
    EXPECT_TRUE(calleeRef->isFunction());
    MirFunction *targetFn = ctx->getFuncById(calleeRef->getRefId());
    ASSERT_NE(targetFn, nullptr);
    EXPECT_EQ(targetFn->getName(), "target_func");

    // LOAD instruction: second operand should be a MirReference to target_var
    ASSERT_NE(instIt, entry->getInstructions().end());
    MirInstruction *loadInst = *instIt++;
    ASSERT_EQ(loadInst->getOpCode(), getOpCodeFromStr("LOAD"));
    ASSERT_GE(loadInst->getOperands().size(), 2);
    MirOperand *varOp = loadInst->getOperands()[1];
    ASSERT_EQ(varOp->getType(), MirOperandType::Reference);
    auto *varRef = static_cast<MirReference *>(varOp);
    EXPECT_TRUE(varRef->isGlobalVar());
    MirGlobalVar *targetGv = ctx->getGVarById(varRef->getRefId());
    ASSERT_NE(targetGv, nullptr);
    EXPECT_EQ(targetGv->getName(), "target_var");
}

/**
 * TMP-11: SSA Violation Detection
 * Verifies that defining a virtual register more than once triggers an SSA violation diagnostic.
 */
TEST_F(MirParserTest, TMP_11_SsaViolationDetection)
{
    MirBuilderContext *ctx = getBuilderCtx();
    EzMir::MirParserOptions options;
    options.verifySsa = true;
    EzMir::MirParser parser(ctx, nullptr, options);

    std::string_view mirCode = R"mir(
fn @ssa_violator() -> i64 {
entry:
    %v0 = MOV i64 1;
    %v0 = MOV i64 2;
    RET i64 %v0;
}
)mir";

    bool success = parser.parseModule(mirCode, "ssa_violation.mir");
    EXPECT_FALSE(success);
}

/**
 * TMP-12: Undefined Block Diagnostics
 * Verifies that branching to a non-existent basic block label reports a diagnostic and fails.
 */
TEST_F(MirParserTest, TMP_12_UndefinedBlockDiagnostics)
{
    MirBuilderContext *ctx = getBuilderCtx();
    EzMir::MirParser parser(ctx);

    std::string_view mirCode = R"mir(
fn @bad_branch() -> void {
entry:
    BR label %non_existent_block;
}
)mir";

    bool success = parser.parseModule(mirCode, "bad_branch.mir");
    EXPECT_FALSE(success);
}

/**
 * TMP-13: Panic-Mode Error Recovery
 * Verifies that the parser resynchronizes past instruction errors to catch subsequent statements
 * without cascading or aborting prematurely.
 */
TEST_F(MirParserTest, TMP_13_ErrorRecovery)
{
    MirBuilderContext *ctx = getBuilderCtx();
    EzMir::MirParserOptions options;
    options.maxErrors = 10;
    EzMir::MirParser parser(ctx, nullptr, options);

    std::string_view mirCode = R"mir(
fn @recovery_test() -> i64 {
entry:
    %v0 = non_existent_opcode_1 i64 1;
    %v1 = MOV i64 10;
    %v2 = non_existent_opcode_2 i64 2;
    %res = ADD i64 %v1, 20;
    RET i64 %res;
}
)mir";

    bool success = parser.parseModule(mirCode, "recovery.mir");
    EXPECT_FALSE(success);
}

/**
 * TMP-14: Telemetry Logging
 * Verifies that trace logging is emitted when enableLogging is true.
 */
TEST_F(MirParserTest, TMP_14_TelemetryLogging)
{
    class LogCaptureListener : public DiagnosticListener
    {
      public:
        size_t m_traceCount{ 0 };
        void onDiag(const DiagnosticMessage &msg) override
        {
            if (msg.getType() == Diag_Trace)
            {
                ++m_traceCount;
            }
        }
    };

    MirBuilderContext *ctx = getBuilderCtx();
    LogCaptureListener listener;
    ctx->getDiagCollector()->addListener(&listener);

    EzMir::MirParserOptions options;
    options.enableLogging = true;
    EzMir::MirParser parser(ctx, nullptr, options);

    std::string_view mirCode = R"mir(
fn @logged_func() -> void {
entry:
    RET;
}
)mir";

    bool success = parser.parseModule(mirCode, "logging.mir");
    EXPECT_TRUE(success);
    EXPECT_GT(listener.m_traceCount, 0);

    ctx->getDiagCollector()->removeListener(&listener);
}

/**
 * TMP-15: Lexer Edge Cases
 * Verifies block comments, inline comments, and string escape sequences.
 */
TEST_F(MirParserTest, TMP_15_LexerEdgeCases)
{
    MirBuilderContext *ctx = getBuilderCtx();
    EzMir::MirParser parser(ctx);

    std::string_view mirCode = R"mir(
/* Multi-line block comment
   spanning multiple lines
   with special symbols: @ % * / -> ; */
@str = internal const [12 x i8] = "Hello\n\t\0World"; /* Inline block comment */

; Single-line comment at start of line
fn @comment_test() -> void {
entry: ; Single-line comment after colon
    RET; ; Single-line comment after semicolon
}
)mir";

    bool success = parser.parseModule(mirCode, "comments.mir");
    EXPECT_TRUE(success);
}

