#include "EzDslTestSuite.h"
#include "Ast/IrInstructionDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Parser/IrInstructionDefLang.h"
#include "Parser/ParseContext.h"
#include "SourceManager/SourceManager.h"

/**
 * Test fixture for IR Instruction Definition Language (.irdf) parser, operand constraints, and AST validation.
 */
class IrInstDefLangTest : public DslTestSuiteAsGtest
{
  public:
};

// ============================================================================
// 1. Operand Parsing Tests
// ============================================================================

/**
 * Verifies parsing an input argument operand with AnyValue type constraint (e.g. AnyValue:src IN).
 */
TEST_F(IrInstDefLangTest, TestIrOperandSimpleIn)
{
    std::string test = "AnyValue:src IN";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::IrInstDef::IrOperand, DSL::Ast::IrInstDef::IrOperand>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_type, DSL::Ast::IrInstDef::IrOperandType::AnyValue);
    EXPECT_EQ(res->m_name.m_node, "src");
    EXPECT_EQ(res->m_dir, DSL::Ast::IrInstDef::IrOperandDir::ArgIn);
}

/**
 * Verifies parsing an output argument operand with Register type constraint (e.g. Register:dst OUT).
 */
TEST_F(IrInstDefLangTest, TestIrOperandSimpleOut)
{
    std::string test = "Register:dst OUT";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::IrInstDef::IrOperand, DSL::Ast::IrInstDef::IrOperand>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_type, DSL::Ast::IrInstDef::IrOperandType::Register);
    EXPECT_EQ(res->m_name.m_node, "dst");
    EXPECT_EQ(res->m_dir, DSL::Ast::IrInstDef::IrOperandDir::ArgOut);
}

/**
 * Verifies parsing composite and bidirectional operand types including RegImm, AddressSource, and INOUT directions.
 */
TEST_F(IrInstDefLangTest, TestIrOperandCompositeTypes)
{
    {
        std::string test = "RegImm:rhs IN";
        ParseContext ctx = createParseContextFromBuff("test", test);
        auto res = ctx.parse<DSL::Parser::IrInstDef::IrOperand, DSL::Ast::IrInstDef::IrOperand>();
        ASSERT_TRUE(res.has_value());
        EXPECT_EQ(res->m_type, DSL::Ast::IrInstDef::IrOperandType::RegImm);
        EXPECT_EQ(res->m_name.m_node, "rhs");
        EXPECT_EQ(res->m_dir, DSL::Ast::IrInstDef::IrOperandDir::ArgIn);
    }

    {
        std::string test = "AddressSource:addr IN";
        ParseContext ctx = createParseContextFromBuff("test2", test);
        auto res = ctx.parse<DSL::Parser::IrInstDef::IrOperand, DSL::Ast::IrInstDef::IrOperand>();
        ASSERT_TRUE(res.has_value());
        EXPECT_EQ(res->m_type, DSL::Ast::IrInstDef::IrOperandType::AddressSource);
        EXPECT_EQ(res->m_name.m_node, "addr");
    }

    {
        std::string test = "Register:val INOUT";
        ParseContext ctx = createParseContextFromBuff("test3", test);
        auto res = ctx.parse<DSL::Parser::IrInstDef::IrOperand, DSL::Ast::IrInstDef::IrOperand>();
        ASSERT_TRUE(res.has_value());
        EXPECT_EQ(res->m_type, DSL::Ast::IrInstDef::IrOperandType::Register);
        EXPECT_EQ(res->m_dir, DSL::Ast::IrInstDef::IrOperandDir::ArgInOut);
    }
}

// ============================================================================
// 2. Single Instruction Declaration Tests
// ============================================================================

/**
 * Verifies parsing zero-operand IR instruction declarations (e.g. NOP).
 */
TEST_F(IrInstDefLangTest, TestInstructionNoOperands)
{
    std::string test = R"dsl(
ir_inst NOP() {
    CATEGORY(System);
    TIER(HighLevel);
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::IrInstDef::IrInstDecl, DSL::Ast::IrInstDef::IrInstDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "NOP");
    EXPECT_TRUE(res->m_operands.empty());
    EXPECT_EQ(res->m_body.m_category, DSL::Ast::IrInstDef::IrInstCategory::System);
    EXPECT_EQ(res->m_body.m_tier, DSL::Ast::IrInstDef::IrInstTier::HighLevel);
    EXPECT_TRUE(res->m_body.m_flags.empty());
}

/**
 * Verifies parsing multi-operand instructions with categories, tiers, and behavioral flag lists.
 */
TEST_F(IrInstDefLangTest, TestInstructionWithOperandsAndFlags)
{
    std::string test = R"dsl(
ir_inst ADD(Register:dst OUT, Register:lhs IN, RegImm:rhs IN) {
    CATEGORY(Arithmetic);
    TIER(HighLevel);
    FLAGS(SizeMatch, IsCommutative);
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::IrInstDef::IrInstDecl, DSL::Ast::IrInstDef::IrInstDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "ADD");

    // Operands
    ASSERT_EQ(res->m_operands.size(), 3);
    EXPECT_EQ(res->m_operands[0].m_type, DSL::Ast::IrInstDef::IrOperandType::Register);
    EXPECT_EQ(res->m_operands[0].m_name.m_node, "dst");
    EXPECT_EQ(res->m_operands[0].m_dir, DSL::Ast::IrInstDef::IrOperandDir::ArgOut);

    EXPECT_EQ(res->m_operands[1].m_type, DSL::Ast::IrInstDef::IrOperandType::Register);
    EXPECT_EQ(res->m_operands[1].m_name.m_node, "lhs");
    EXPECT_EQ(res->m_operands[1].m_dir, DSL::Ast::IrInstDef::IrOperandDir::ArgIn);

    EXPECT_EQ(res->m_operands[2].m_type, DSL::Ast::IrInstDef::IrOperandType::RegImm);
    EXPECT_EQ(res->m_operands[2].m_name.m_node, "rhs");
    EXPECT_EQ(res->m_operands[2].m_dir, DSL::Ast::IrInstDef::IrOperandDir::ArgIn);

    // Body
    EXPECT_EQ(res->m_body.m_category, DSL::Ast::IrInstDef::IrInstCategory::Arithmetic);
    EXPECT_EQ(res->m_body.m_tier, DSL::Ast::IrInstDef::IrInstTier::HighLevel);
    ASSERT_EQ(res->m_body.m_flags.size(), 2);
    EXPECT_EQ(res->m_body.m_flags[0], DSL::Ast::IrInstDef::IrInstFlag::SizeMatch);
    EXPECT_EQ(res->m_body.m_flags[1], DSL::Ast::IrInstDef::IrInstFlag::IsCommutative);
}

/**
 * Verifies parsing instructions with body attributes declared in arbitrary order.
 */
TEST_F(IrInstDefLangTest, TestInstructionArbitraryBodyOrder)
{
    std::string test = R"dsl(
ir_inst STORE(AddressSource:dst IN, AnyValue:src IN) {
    FLAGS(WritesMemory, HasSideEffect);
    TIER(HighLevel);
    CATEGORY(Memory);
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::IrInstDef::IrInstDecl, DSL::Ast::IrInstDef::IrInstDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "STORE");

    ASSERT_EQ(res->m_operands.size(), 2);
    EXPECT_EQ(res->m_operands[0].m_name.m_node, "dst");
    EXPECT_EQ(res->m_operands[1].m_name.m_node, "src");

    EXPECT_EQ(res->m_body.m_category, DSL::Ast::IrInstDef::IrInstCategory::Memory);
    EXPECT_EQ(res->m_body.m_tier, DSL::Ast::IrInstDef::IrInstTier::HighLevel);
    ASSERT_EQ(res->m_body.m_flags.size(), 2);
    EXPECT_EQ(res->m_body.m_flags[0], DSL::Ast::IrInstDef::IrInstFlag::WritesMemory);
    EXPECT_EQ(res->m_body.m_flags[1], DSL::Ast::IrInstDef::IrInstFlag::HasSideEffect);
}

/**
 * Verifies parsing compiler-internal pass instructions (e.g. POP_RET with PassInternal tier).
 */
TEST_F(IrInstDefLangTest, TestInternalPassInstruction)
{
    std::string test = R"dsl(
ir_inst POP_RET(Register:token IN, Register:dst OUT) {
    CATEGORY(DataMovement);
    TIER(PassInternal);
    FLAGS(HasSideEffect);
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::IrInstDef::IrInstDecl, DSL::Ast::IrInstDef::IrInstDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "POP_RET");
    EXPECT_EQ(res->m_body.m_category, DSL::Ast::IrInstDef::IrInstCategory::DataMovement);
    EXPECT_EQ(res->m_body.m_tier, DSL::Ast::IrInstDef::IrInstTier::PassInternal);
    ASSERT_EQ(res->m_body.m_flags.size(), 1);
    EXPECT_EQ(res->m_body.m_flags[0], DSL::Ast::IrInstDef::IrInstFlag::HasSideEffect);
}

// ============================================================================
// 3. Full File / Translation Unit Tests
// ============================================================================

/**
 * Verifies parsing an entire .irdf file with multiple instruction declarations across data movement, memory, and control flow.
 */
TEST_F(IrInstDefLangTest, TestMultipleInstructionsInFile)
{
    std::string test = R"dsl(
ir_inst MOV(Register:dst OUT, AnyValue:src IN) {
    CATEGORY(DataMovement);
    TIER(HighLevel);
}

ir_inst LOAD(Register:dst OUT, AddressSource:src IN) {
    CATEGORY(Memory);
    TIER(HighLevel);
    FLAGS(ReadsMemory);
}

ir_inst BR_COND(Register:cond IN, Reference:trueBlock IN, Reference:falseBlock IN) {
    CATEGORY(ControlFlow);
    TIER(HighLevel);
    FLAGS(IsTerminator, IsBranch);
}

ir_inst CALL(Register:dstRet OUT, AnyValue:target IN) {
    CATEGORY(ControlFlow);
    TIER(HighLevel);
    FLAGS(IsCall, HasSideEffect, VariadicArgs);
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::IrInstDef::IrInstDefFile, DSL::Ast::IrInstDef::IrInstDefFile>();
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->m_instructions.size(), 4);

    EXPECT_EQ(res->m_instructions[0].m_name.m_node, "MOV");
    EXPECT_EQ(res->m_instructions[0].m_body.m_category, DSL::Ast::IrInstDef::IrInstCategory::DataMovement);

    EXPECT_EQ(res->m_instructions[1].m_name.m_node, "LOAD");
    EXPECT_EQ(res->m_instructions[1].m_body.m_category, DSL::Ast::IrInstDef::IrInstCategory::Memory);
    ASSERT_EQ(res->m_instructions[1].m_body.m_flags.size(), 1);
    EXPECT_EQ(res->m_instructions[1].m_body.m_flags[0], DSL::Ast::IrInstDef::IrInstFlag::ReadsMemory);

    EXPECT_EQ(res->m_instructions[2].m_name.m_node, "BR_COND");
    EXPECT_EQ(res->m_instructions[2].m_operands.size(), 3);
    EXPECT_EQ(res->m_instructions[2].m_body.m_category, DSL::Ast::IrInstDef::IrInstCategory::ControlFlow);

    EXPECT_EQ(res->m_instructions[3].m_name.m_node, "CALL");
    ASSERT_EQ(res->m_instructions[3].m_body.m_flags.size(), 3);
    EXPECT_EQ(res->m_instructions[3].m_body.m_flags[0], DSL::Ast::IrInstDef::IrInstFlag::IsCall);
    EXPECT_EQ(res->m_instructions[3].m_body.m_flags[1], DSL::Ast::IrInstDef::IrInstFlag::HasSideEffect);
    EXPECT_EQ(res->m_instructions[3].m_body.m_flags[2], DSL::Ast::IrInstDef::IrInstFlag::VariadicArgs);
}

// ============================================================================
// 4. Negative & Error Parsing Tests
// ============================================================================

/**
 * Verifies syntax error rejection when an unknown operand type is specified.
 */
TEST_F(IrInstDefLangTest, TestUnknownOperandTypeFails)
{
    std::string test = R"dsl(
ir_inst BAD(UnknownType:dst OUT) {
    CATEGORY(Arithmetic);
    TIER(HighLevel);
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::IrInstDef::IrInstDefFile, DSL::Ast::IrInstDef::IrInstDefFile>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when an operand is missing its direction specification.
 */
TEST_F(IrInstDefLangTest, TestMissingDirectionFails)
{
    std::string test = R"dsl(
ir_inst BAD(Register:dst) {
    CATEGORY(Arithmetic);
    TIER(HighLevel);
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::IrInstDef::IrInstDefFile, DSL::Ast::IrInstDef::IrInstDefFile>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when an operand is missing the colon separator.
 */
TEST_F(IrInstDefLangTest, TestMissingColonInOperandFails)
{
    std::string test = R"dsl(
ir_inst BAD(Register dst OUT) {
    CATEGORY(Arithmetic);
    TIER(HighLevel);
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::IrInstDef::IrInstDefFile, DSL::Ast::IrInstDef::IrInstDefFile>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when an unknown category identifier is specified.
 */
TEST_F(IrInstDefLangTest, TestUnknownCategoryFails)
{
    std::string test = R"dsl(
ir_inst BAD(Register:dst OUT) {
    CATEGORY(QuantumOps);
    TIER(HighLevel);
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::IrInstDef::IrInstDefFile, DSL::Ast::IrInstDef::IrInstDefFile>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when a semicolon is missing in the instruction body.
 */
TEST_F(IrInstDefLangTest, TestMissingSemicolonInBodyItemFails)
{
    std::string test = R"dsl(
ir_inst BAD(Register:dst OUT) {
    CATEGORY(Arithmetic)
    TIER(HighLevel);
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::IrInstDef::IrInstDefFile, DSL::Ast::IrInstDef::IrInstDefFile>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when instruction body closing braces are missing.
 */
TEST_F(IrInstDefLangTest, TestUnclosedBracesFails)
{
    std::string test = R"dsl(
ir_inst BAD(Register:dst OUT) {
    CATEGORY(Arithmetic);
    TIER(HighLevel);
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::IrInstDef::IrInstDefFile, DSL::Ast::IrInstDef::IrInstDefFile>();
    EXPECT_FALSE(res.has_value());
}