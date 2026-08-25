#include "EzDslTestSuite.h"
#include "Ast/InstructionDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Parser/InstructionDefLang.h"
#include "Parser/ParseContext.h"
#include "SourceManager/SourceManager.h"

class InstDefLangTest : public DslTestSuiteAsGtest
{
  public:
};

// ============================================================================
// 1. Bit Slices & Bit Expressions
// ============================================================================

TEST_F(InstDefLangTest, TestBitSliceNormalization)
{
    std::string test = "[31:0]";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstDef::BitSlice, DSL::Ast::InstDef::BitSlice>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_to, 31);
    EXPECT_EQ(res->m_from, 0);

    // Test reverse index specification [0:15]
    std::string testRev = "[0:15]";
    ParseContext ctxRev = createParseContextFromBuff("testRev", testRev);

    auto resRev = ctxRev.parse<DSL::Parser::InstDef::BitSlice, DSL::Ast::InstDef::BitSlice>();
    ASSERT_TRUE(resRev.has_value());
    EXPECT_EQ(resRev->m_from, 0);
    EXPECT_EQ(resRev->m_to, 15);
}

TEST_F(InstDefLangTest, TestSlicedIdentifier)
{
    std::string test = "imm12[0:4]";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstDef::SlicedIdentifier, DSL::Ast::InstDef::SlicedIdentifier>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "imm12");
    EXPECT_EQ(res->m_slice.m_from, 0);
    EXPECT_EQ(res->m_slice.m_to, 4);
}

TEST_F(InstDefLangTest, TestBitExpressionPrecedence)
{
    // Evaluates: a | (b & (c << 2))
    std::string test = "a | b & c << 2";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstDef::BitExpression, DSL::Ast::InstDef::BitExprValues>();
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(std::holds_alternative<DSL::Ast::InstDef::BitExpression*>(*res));

    // Root should be Bitwise OR (lowest precedence)
    const auto &rootExpr = std::get<DSL::Ast::InstDef::BitExpression*>(*res);
    EXPECT_EQ(rootExpr->m_op, DSL::Ast::InstDef::BitExprOp::Or);
    ASSERT_TRUE(std::holds_alternative<DSL::Ast::Common::Identifier>(rootExpr->m_lhs));
    EXPECT_EQ(std::get<DSL::Ast::Common::Identifier>(rootExpr->m_lhs).m_node, "a");

    // RHS of OR should be Bitwise AND
    ASSERT_TRUE(rootExpr->m_rhs.has_value());
    ASSERT_TRUE(std::holds_alternative<DSL::Ast::InstDef::BitExpression*>(*rootExpr->m_rhs));
    const auto &andExpr = std::get<DSL::Ast::InstDef::BitExpression*>(*rootExpr->m_rhs);
    EXPECT_EQ(andExpr->m_op, DSL::Ast::InstDef::BitExprOp::And);

    // RHS of AND should be SHL (<<)
    ASSERT_TRUE(andExpr->m_rhs.has_value());
    ASSERT_TRUE(std::holds_alternative<DSL::Ast::InstDef::BitExpression*>(*andExpr->m_rhs));
    const auto &shlExpr = std::get<DSL::Ast::InstDef::BitExpression*>(*andExpr->m_rhs);
    EXPECT_EQ(shlExpr->m_op, DSL::Ast::InstDef::BitExprOp::Shl);
}

TEST_F(InstDefLangTest, TestUnaryComplementAndSliceInExpression)
{
    std::string test = "~mask & imm12[0:4]";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstDef::BitExpression, DSL::Ast::InstDef::BitExprValues>();
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(std::holds_alternative<DSL::Ast::InstDef::BitExpression*>(*res));

    const auto &root = std::get<DSL::Ast::InstDef::BitExpression*>(*res);
    EXPECT_EQ(root->m_op, DSL::Ast::InstDef::BitExprOp::And);

    // Unary NOT check on LHS
    ASSERT_TRUE(std::holds_alternative<DSL::Ast::InstDef::BitExpression*>(root->m_lhs));
    const auto &notExpr = std::get<DSL::Ast::InstDef::BitExpression*>(root->m_lhs);
    EXPECT_EQ(notExpr->m_op, DSL::Ast::InstDef::BitExprOp::Not);
    EXPECT_FALSE(notExpr->m_rhs.has_value());

    // Sliced identifier on RHS
    ASSERT_TRUE(root->m_rhs.has_value());
    ASSERT_TRUE(std::holds_alternative<DSL::Ast::InstDef::SlicedIdentifier>(*root->m_rhs));
    const auto &sliced = std::get<DSL::Ast::InstDef::SlicedIdentifier>(*root->m_rhs);
    EXPECT_EQ(sliced.m_name.m_node, "imm12");
    EXPECT_EQ(sliced.m_slice.m_from, 0);
    EXPECT_EQ(sliced.m_slice.m_to, 4);
}

TEST_F(InstDefLangTest, TestBitAssignment)
{
    std::string test = "imm4_0[0:4] = imm12[0:4]";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstDef::BitExprAssign, DSL::Ast::InstDef::BitExprAssign>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_lhs.m_node, "imm4_0");
    ASSERT_TRUE(res->m_lhsSlice.has_value());
    EXPECT_EQ(res->m_lhsSlice->m_from, 0);
    EXPECT_EQ(res->m_lhsSlice->m_to, 4);

    ASSERT_TRUE(std::holds_alternative<DSL::Ast::InstDef::SlicedIdentifier>(res->m_rhs));
    const auto &rhsExpr = std::get<DSL::Ast::InstDef::SlicedIdentifier>(res->m_rhs);
    EXPECT_EQ(rhsExpr.m_name.m_node, "imm12");
    EXPECT_EQ(rhsExpr.m_slice.m_from, 0);
    EXPECT_EQ(rhsExpr.m_slice.m_to, 4);
}

// ============================================================================
// 2. Formats & Layouts
// ============================================================================

TEST_F(InstDefLangTest, TestFormatDeclarationExplicitWidth)
{
    std::string test = R"(
format RType(32) {
    opcode[0:6];
    rd[7:11];
    funct3[12:14];
    rs1[15:19];
    rs2[20:24];
    funct7[25:31];
    funct8[32:37];
};
)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstDef::InstFormatDecl, DSL::Ast::InstDef::InstFormatDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "RType");
    EXPECT_EQ(res->m_bitWidth, 32);
    ASSERT_EQ(res->m_fields.size(), 7);

    EXPECT_EQ(res->m_fields[0].m_name.m_node, "opcode");
    EXPECT_EQ(res->m_fields[0].m_slice.m_from, 0);
    EXPECT_EQ(res->m_fields[0].m_slice.m_to, 6);

    EXPECT_EQ(res->m_fields[5].m_name.m_node, "funct7");
    EXPECT_EQ(res->m_fields[5].m_slice.m_from, 25);
    EXPECT_EQ(res->m_fields[5].m_slice.m_to, 31);

    EXPECT_EQ(res->m_fields[6].m_name.m_node, "funct8");
    EXPECT_EQ(res->m_fields[6].m_slice.m_from, 32);
    EXPECT_EQ(res->m_fields[6].m_slice.m_to, 37);
}

TEST_F(InstDefLangTest, TestFormatDeclarationDefaultWidth)
{
    std::string test = R"(
format SimpleFormat {
    fieldA[0:15];
    fieldB[16:31];
};
)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstDef::InstFormatDecl, DSL::Ast::InstDef::InstFormatDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "SimpleFormat");
    EXPECT_EQ(res->m_bitWidth, 32);
    ASSERT_EQ(res->m_fields.size(), 2);
}

// ============================================================================
// 3. Unified Instruction Operands & Header
// ============================================================================

TEST_F(InstDefLangTest, TestRegisterAndImmediateOperands)
{
    // 1. Register operand
    std::string regTest = "GPR:rd OUT";
    ParseContext regCtx = createParseContextFromBuff("regTest", regTest);

    auto regRes = regCtx.parse<DSL::Parser::InstDef::InstOperand, DSL::Ast::InstDef::InstOperand>();
    ASSERT_TRUE(regRes.has_value());
    EXPECT_EQ(regRes->m_kind, DSL::Ast::InstDef::InstOperandKind::Register);
    EXPECT_EQ(regRes->m_typeOrClass.m_node, "GPR");
    EXPECT_FALSE(regRes->m_typeParam.has_value());
    EXPECT_EQ(regRes->m_name.m_node, "rd");
    EXPECT_EQ(regRes->m_dir, DSL::Ast::InstDef::InstOperandDir::ArgOut);

    // 2. Parameterized immediate operand (e.g., simm(i12):offset IN)
    std::string immParamTest = "simm(i12):offset IN";
    ParseContext immParamCtx = createParseContextFromBuff("immParamTest", immParamTest);

    auto immParamRes = immParamCtx.parse<DSL::Parser::InstDef::InstOperand, DSL::Ast::InstDef::InstOperand>();
    ASSERT_TRUE(immParamRes.has_value());
    EXPECT_EQ(immParamRes->m_kind, DSL::Ast::InstDef::InstOperandKind::Immediate);
    EXPECT_EQ(immParamRes->m_typeOrClass.m_node, "simm");
    ASSERT_TRUE(immParamRes->m_typeParam.has_value());
    EXPECT_EQ(immParamRes->m_typeParam->m_node, "i12");
    EXPECT_EQ(immParamRes->m_name.m_node, "offset");
    EXPECT_EQ(immParamRes->m_dir, DSL::Ast::InstDef::InstOperandDir::ArgIn);

    // 3. Unparameterized immediate operand (e.g., imm:val IN)
    std::string immTest = "imm:val IN";
    ParseContext immCtx = createParseContextFromBuff("immTest", immTest);

    auto immRes = immCtx.parse<DSL::Parser::InstDef::InstOperand, DSL::Ast::InstDef::InstOperand>();
    ASSERT_TRUE(immRes.has_value());
    EXPECT_EQ(immRes->m_kind, DSL::Ast::InstDef::InstOperandKind::Immediate);
    EXPECT_EQ(immRes->m_typeOrClass.m_node, "imm");
    EXPECT_FALSE(immRes->m_typeParam.has_value());
    EXPECT_EQ(immRes->m_name.m_node, "val");
    EXPECT_EQ(immRes->m_dir, DSL::Ast::InstDef::InstOperandDir::ArgIn);
}

TEST_F(InstDefLangTest, TestInstHeader)
{
    std::string test = "inst SW(GPR:rs2 IN, GPR:rs1 IN, simm(i12):imm12 IN) format SType";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstDef::InstHeader, DSL::Ast::InstDef::InstHeader>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "SW");
    EXPECT_EQ(res->m_formatName.m_node, "SType");
    ASSERT_EQ(res->m_args.size(), 3);

    // rs2
    EXPECT_EQ(res->m_args[0].m_kind, DSL::Ast::InstDef::InstOperandKind::Register);
    EXPECT_EQ(res->m_args[0].m_typeOrClass.m_node, "GPR");
    EXPECT_EQ(res->m_args[0].m_name.m_node, "rs2");
    EXPECT_EQ(res->m_args[0].m_dir, DSL::Ast::InstDef::InstOperandDir::ArgIn);

    // imm12
    EXPECT_EQ(res->m_args[2].m_kind, DSL::Ast::InstDef::InstOperandKind::Immediate);
    EXPECT_EQ(res->m_args[2].m_typeOrClass.m_node, "simm");
    ASSERT_TRUE(res->m_args[2].m_typeParam.has_value());
    EXPECT_EQ(res->m_args[2].m_typeParam->m_node, "i12");
    EXPECT_EQ(res->m_args[2].m_name.m_node, "imm12");
    EXPECT_EQ(res->m_args[2].m_dir, DSL::Ast::InstDef::InstOperandDir::ArgIn);
}

// ============================================================================
// 4. Complete Instruction Declarations & Translation Unit
// ============================================================================

TEST_F(InstDefLangTest, TestCompleteInstructionDeclaration)
{
    std::string test = R"(
inst ADD(GPR:rd OUT, GPR:rs1 IN, GPR:rs2 IN) format RType {
    IMPLICIT(CSR:fcsr INOUT);
    FORMAT(
        opcode = 0x33,
        funct3 = 0,
        funct7 = 0
    );
    FLAGS(commutative);
    ASM("add $rd, $rs1, $rs2");
    LATENCY(1);
}
)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstDef::InstDecl, DSL::Ast::InstDef::InstDecl>();
    ASSERT_TRUE(res.has_value());

    // Header validation
    EXPECT_EQ(res->m_header.m_name.m_node, "ADD");
    EXPECT_EQ(res->m_header.m_formatName.m_node, "RType");
    ASSERT_EQ(res->m_header.m_args.size(), 3);

    // Implicit arguments
    ASSERT_EQ(res->m_body.m_implicitArgs.size(), 1);
    EXPECT_EQ(res->m_body.m_implicitArgs[0].m_kind, DSL::Ast::InstDef::InstOperandKind::Register);
    EXPECT_EQ(res->m_body.m_implicitArgs[0].m_typeOrClass.m_node, "CSR");
    EXPECT_EQ(res->m_body.m_implicitArgs[0].m_name.m_node, "fcsr");
    EXPECT_EQ(res->m_body.m_implicitArgs[0].m_dir, DSL::Ast::InstDef::InstOperandDir::ArgInOut);

    // Format assignments
    ASSERT_EQ(res->m_body.m_assigns.size(), 3);
    EXPECT_EQ(res->m_body.m_assigns[0].m_lhs.m_node, "opcode");
    ASSERT_TRUE(std::holds_alternative<DSL::Ast::Common::IntegerLiteral>(res->m_body.m_assigns[0].m_rhs));
    const auto &opExpr = std::get<DSL::Ast::Common::IntegerLiteral>(res->m_body.m_assigns[0].m_rhs);
    EXPECT_EQ(opExpr.m_node, 0x33);

    // Metadata
    ASSERT_EQ(res->m_body.m_flags.size(), 1);
    EXPECT_EQ(res->m_body.m_flags[0], DSL::Ast::InstDef::InstFlag::IsCommutative);
    EXPECT_EQ(res->m_body.m_asmTemplate, "add $rd, $rs1, $rs2");
    EXPECT_EQ(res->m_body.m_latency, 1);
}

TEST_F(InstDefLangTest, TestFullTranslationUnit)
{
    std::string test = R"dsl(
format SType(32) {
    opcode[0:6];
    imm4_0[7:11];
    funct3[12:14];
    rs1[15:19];
    rs2[20:24];
    imm11_5[25:31];
};

format RType(32) {
    opcode[0:6];
    rd[7:11];
    funct3[12:14];
    rs1[15:19];
    rs2[20:24];
    funct7[25:31];
};

inst SW(GPR:rs2 IN, GPR:rs1 IN, simm(i12):imm12 IN) format SType {
    FORMAT(
        opcode = 0x23,
        funct3 = 2,
        imm4_0 = imm12[0:4],
        imm11_5 = imm12[5:11]
    );
    FLAGS(mayStore);
    ASM("sw $rs2, ${imm12}(${rs1})");
    LATENCY(1);
}

inst ADD(GPR:rd OUT, GPR:rs1 IN, GPR:rs2 IN) format RType {
    IMPLICIT(CSR:fcsr INOUT);
    FORMAT(opcode = 0x33, funct3 = 0, funct7 = 0);
    FLAGS(commutative);
    ASM("add $rd, $rs1, $rs2");
    LATENCY(1);
}
)dsl";

    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstDef::InstDefFile, DSL::Ast::InstDef::InstDefFile>();
    ASSERT_TRUE(res.has_value());

    EXPECT_EQ(res->m_formats.size(), 2);
    EXPECT_EQ(res->m_instructions.size(), 2);

    EXPECT_EQ(res->m_formats[0].m_name.m_node, "SType");
    EXPECT_EQ(res->m_formats[1].m_name.m_node, "RType");

    EXPECT_EQ(res->m_instructions[0].m_header.m_name.m_node, "SW");
    EXPECT_EQ(res->m_instructions[1].m_header.m_name.m_node, "ADD");
}

// ============================================================================
// 5. Negative & Error Parsing Tests
// ============================================================================

TEST_F(InstDefLangTest, TestMissingSemicolonInFormatError)
{
    std::string test = R"(
format BadFormat {
    fieldA[0:15]
    fieldB[16:31];
};
)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstDef::InstFormatDecl, DSL::Ast::InstDef::InstFormatDecl>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(InstDefLangTest, TestInvalidDirectionError)
{
    std::string test = "GPR:rd INVALIDSIDE";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstDef::InstOperand, DSL::Ast::InstDef::InstOperand>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(InstDefLangTest, TestUnterminatedInstBodyError)
{
    std::string test = R"(
inst ADD(GPR:rd OUT, GPR:rs1 IN) format RType {
    ASM("add $rd, $rs1");
    LATENCY(1);
)"; // Missing closing brace '}'

    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstDef::InstDecl, DSL::Ast::InstDef::InstDecl>();
    EXPECT_FALSE(res.has_value());
}