#include "EzDslSemaTestSuite.h"
#include "Ast/IrInstructionDefLangAst.h"
#include "Ast/TypeDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/IrInstructionDefLang.h"
#include "Parser/ParseContext.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/IrInstructionPass.h"

/**
 * Test fixture for semantic analysis of IR Instruction Definition Language (.irdf) AST.
 */
class IrInstructionPassTest : public EzDslSemaTestSuiteAsGtest
{
  protected:
    std::optional<DSL::Ast::IrInstDef::IrInstDefFile> parseIrInstDefFile(const std::string &source)
    {
        ParseContext ctx = createParseContextFromBuff(std::format("test_{}.irdf", m_testId++), source);
        return ctx.parse<DSL::Parser::IrInstDef::IrInstDefFile, DSL::Ast::IrInstDef::IrInstDefFile>();
    }

    static bool hasFlag(DSL::Ast::IrInstDef::IrInstFlag combined, DSL::Ast::IrInstDef::IrInstFlag target)
    {
        return (static_cast<uint32_t>(combined) & static_cast<uint32_t>(target)) != 0;
    }

  private:
    size_t m_testId{ 0 };
};

// ============================================================================
// 1. Successful Semantic Registration & Instruction Signatures
// ============================================================================

/**
 * Verifies declaration, semantic validation, and symbol registration of a standard
 * ALU instruction with multiple operands, tier, category, and combined flags.
 */
TEST_F(IrInstructionPassTest, TestValidArithmeticInstruction)
{
    std::string source = R"(
        ir_inst Add ( Register:dst OUT, Register:lhs IN, RegImm:rhs IN ) {
            CATEGORY(Arithmetic);
            TIER(HighLevel);
            FLAGS(SizeMatch, IsCommutative);
        };
    )";

    auto ast = parseIrInstDefFile(source);
    ASSERT_TRUE(ast.has_value());

    IrInstructionPass pass;
    EXPECT_TRUE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));

    Symbol *sym = getSymbolTable()->getSymByName("Add");
    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->getType(), SymbolType::IrInstruction);

    auto *data = sym->getIf<Symbols::IrInstructionSymbol>();
    ASSERT_NE(data, nullptr);
    EXPECT_EQ(data->m_name, "Add");
    EXPECT_EQ(data->m_category, DSL::Ast::IrInstDef::IrInstCategory::Arithmetic);
    EXPECT_EQ(data->m_tier, DSL::Ast::IrInstDef::IrInstTier::HighLevel);
    EXPECT_TRUE(hasFlag(data->m_flags, DSL::Ast::IrInstDef::IrInstFlag::SizeMatch));
    EXPECT_TRUE(hasFlag(data->m_flags, DSL::Ast::IrInstDef::IrInstFlag::IsCommutative));

    ASSERT_EQ(data->m_operands.size(), 3);
    EXPECT_EQ(data->m_operands[0].m_name, "dst");
    EXPECT_EQ(data->m_operands[0].m_type, DSL::Ast::IrInstDef::IrOperandType::Register);
    EXPECT_EQ(data->m_operands[0].m_dir, DSL::Ast::IrInstDef::IrOperandDir::ArgOut);

    EXPECT_EQ(data->m_operands[1].m_name, "lhs");
    EXPECT_EQ(data->m_operands[1].m_type, DSL::Ast::IrInstDef::IrOperandType::Register);
    EXPECT_EQ(data->m_operands[1].m_dir, DSL::Ast::IrInstDef::IrOperandDir::ArgIn);

    EXPECT_EQ(data->m_operands[2].m_name, "rhs");
    EXPECT_EQ(data->m_operands[2].m_type, DSL::Ast::IrInstDef::IrOperandType::RegImm);
    EXPECT_EQ(data->m_operands[2].m_dir, DSL::Ast::IrInstDef::IrOperandDir::ArgIn);
}

/**
 * Verifies valid control-flow instructions (branches with IsTerminator and function calls).
 */
TEST_F(IrInstructionPassTest, TestValidControlFlowInstructions)
{
    std::string source = R"(
        ir_inst Branch ( AddressSource:target IN ) {
            CATEGORY(ControlFlow);
            TIER(HighLevel);
            FLAGS(IsBranch, IsTerminator);
        };

        ir_inst Call ( RuntimeSymbol:target IN, VariadicArgs:args IN ) {
            CATEGORY(ControlFlow);
            TIER(HighLevel);
            FLAGS(IsCall);
        };

        ir_inst Ret () {
            CATEGORY(ControlFlow);
            TIER(HighLevel);
            FLAGS(IsReturn, IsTerminator);
        };
    )";

    auto ast = parseIrInstDefFile(source);
    ASSERT_TRUE(ast.has_value());

    IrInstructionPass pass;
    EXPECT_TRUE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));
    EXPECT_NE(getSymbolTable()->getSymByName("Branch"), nullptr);
    EXPECT_NE(getSymbolTable()->getSymByName("Call"), nullptr);
    EXPECT_NE(getSymbolTable()->getSymByName("Ret"), nullptr);
}

/**
 * Verifies valid casting operations requiring size extension/truncation flags
 * along with at least 1 IN and 1 OUT operand.
 */
TEST_F(IrInstructionPassTest, TestValidCastingInstruction)
{
    std::string source = R"(
        ir_inst ZExt ( Register:dst OUT, Register:src IN ) {
            CATEGORY(Casting);
            TIER(HighLevel);
            FLAGS(DestLarger);
        };

        ir_inst Trunc ( Register:dst OUT, Register:src IN ) {
            CATEGORY(Casting);
            TIER(HighLevel);
            FLAGS(DestSmaller);
        };
    )";

    auto ast = parseIrInstDefFile(source);
    ASSERT_TRUE(ast.has_value());

    IrInstructionPass pass;
    EXPECT_TRUE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));
    EXPECT_NE(getSymbolTable()->getSymByName("ZExt"), nullptr);
    EXPECT_NE(getSymbolTable()->getSymByName("Trunc"), nullptr);
}

/**
 * Verifies pure computational instructions without an OUT register are allowed
 * if explicitly flagged with HasSideEffect.
 */
TEST_F(IrInstructionPassTest, TestPureComputeWithSideEffectAllowed)
{
    std::string source = R"(
        ir_inst TestBit ( Register:src IN, Immediate:bit IN ) {
            CATEGORY(Bitwise);
            TIER(HighLevel);
            FLAGS(HasSideEffect);
        };
    )";

    auto ast = parseIrInstDefFile(source);
    ASSERT_TRUE(ast.has_value());

    IrInstructionPass pass;
    EXPECT_TRUE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));
    EXPECT_NE(getSymbolTable()->getSymByName("TestBit"), nullptr);
}

// ============================================================================
// 2. Operand Semantic Validations
// ============================================================================

/**
 * Verifies rejection when an instruction signature declares duplicate operand identifiers.
 */
TEST_F(IrInstructionPassTest, TestDuplicateOperandNameFails)
{
    std::string source = R"(
        ir_inst BadInst ( Register:op IN, Register:op OUT ) {
            CATEGORY(Arithmetic);
        };
    )";

    auto ast = parseIrInstDefFile(source);
    ASSERT_TRUE(ast.has_value());

    IrInstructionPass pass;
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));
}

/**
 * Verifies rejection when an immediate operand is specified as OUT or INOUT.
 */
TEST_F(IrInstructionPassTest, TestImmediateAsOutputFails)
{
    std::string sourceOut = R"(
        ir_inst BadImmOut ( Immediate:val OUT ) {
            CATEGORY(DataMovement);
        };
    )";

    auto astOut = parseIrInstDefFile(sourceOut);
    ASSERT_TRUE(astOut.has_value());

    IrInstructionPass pass;
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &astOut.value()));

    std::string sourceInOut = R"(
        ir_inst BadImmInOut ( Immediate:val INOUT ) {
            CATEGORY(DataMovement);
        };
    )";

    auto astInOut = parseIrInstDefFile(sourceInOut);
    ASSERT_TRUE(astInOut.has_value());
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &astInOut.value()));
}

// ============================================================================
// 3. Category & Flag Semantic Validations
// ============================================================================

/**
 * Verifies rejection when an instruction definition omits CATEGORY.
 */
TEST_F(IrInstructionPassTest, TestMissingCategoryFails)
{
    std::string source = R"(
        ir_inst NoCategory ( Register:dst OUT ) {
            TIER(HighLevel);
        };
    )";

    auto ast = parseIrInstDefFile(source);
    ASSERT_TRUE(ast.has_value());

    IrInstructionPass pass;
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));
}

/**
 * Verifies that IsBranch and IsReturn require the IsTerminator flag.
 */
TEST_F(IrInstructionPassTest, TestBranchOrReturnWithoutTerminatorFails)
{
    std::string branchSource = R"(
        ir_inst BranchNoTerm ( AddressSource:target IN ) {
            CATEGORY(ControlFlow);
            FLAGS(IsBranch);
        };
    )";

    auto astBranch = parseIrInstDefFile(branchSource);
    ASSERT_TRUE(astBranch.has_value());

    IrInstructionPass pass;
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &astBranch.value()));

    std::string retSource = R"(
        ir_inst RetNoTerm () {
            CATEGORY(ControlFlow);
            FLAGS(IsReturn);
        };
    )";

    auto astRet = parseIrInstDefFile(retSource);
    ASSERT_TRUE(astRet.has_value());
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &astRet.value()));
}

/**
 * Verifies that control flow flags (IsBranch, IsCall, IsReturn) are mutually exclusive.
 */
TEST_F(IrInstructionPassTest, TestMutuallyExclusiveControlFlowFlagsFails)
{
    std::string source = R"(
        ir_inst BranchAndCall () {
            CATEGORY(ControlFlow);
            FLAGS(IsBranch, IsCall, IsTerminator);
        };
    )";

    auto ast = parseIrInstDefFile(source);
    ASSERT_TRUE(ast.has_value());

    IrInstructionPass pass;
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));
}

/**
 * Verifies rejection when IsCommutative is specified on an instruction with fewer than 2 IN operands.
 */
TEST_F(IrInstructionPassTest, TestCommutativeInsufficientInputsFails)
{
    std::string source = R"(
        ir_inst Neg ( Register:dst OUT, Register:src IN ) {
            CATEGORY(Arithmetic);
            FLAGS(IsCommutative);
        };
    )";

    auto ast = parseIrInstDefFile(source);
    ASSERT_TRUE(ast.has_value());

    IrInstructionPass pass;
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));
}

/**
 * Verifies that SizeMatch, DestLarger, and DestSmaller are mutually exclusive.
 */
TEST_F(IrInstructionPassTest, TestMutuallyExclusiveSizeFlagsFails)
{
    std::string source = R"(
        ir_inst BadCast ( Register:dst OUT, Register:src IN ) {
            CATEGORY(Casting);
            FLAGS(DestLarger, DestSmaller);
        };
    )";

    auto ast = parseIrInstDefFile(source);
    ASSERT_TRUE(ast.has_value());

    IrInstructionPass pass;
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));
}

/**
 * Verifies that casting flags (DestLarger / DestSmaller) require at least 1 IN and 1 OUT operand.
 */
TEST_F(IrInstructionPassTest, TestCastingMissingOperandsFails)
{
    std::string source = R"(
        ir_inst CastNoOutput ( Register:src IN ) {
            CATEGORY(Casting);
            FLAGS(DestLarger, HasSideEffect);
        };
    )";

    auto ast = parseIrInstDefFile(source);
    ASSERT_TRUE(ast.has_value());

    IrInstructionPass pass;
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));
}

/**
 * Verifies rejection of pure compute ops (Arithmetic, Bitwise, Compare, Casting)
 * that produce no output register and lack HasSideEffect.
 */
TEST_F(IrInstructionPassTest, TestDeadPureComputeFails)
{
    std::string source = R"(
        ir_inst DeadMath ( Register:lhs IN, Register:rhs IN ) {
            CATEGORY(Arithmetic);
        };
    )";

    auto ast = parseIrInstDefFile(source);
    ASSERT_TRUE(ast.has_value());

    IrInstructionPass pass;
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));
}

// ============================================================================
// 4. Duplicate Symbol & Redeclaration Validations
// ============================================================================

/**
 * Verifies that duplicate IR instruction definitions within the same compilation unit fail.
 */
TEST_F(IrInstructionPassTest, TestDuplicateInstructionNameFails)
{
    std::string source = R"(
        ir_inst Nop () {
            CATEGORY(System);
        };

        ir_inst Nop () {
            CATEGORY(System);
        };
    )";

    auto ast = parseIrInstDefFile(source);
    ASSERT_TRUE(ast.has_value());

    IrInstructionPass pass;
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));
}

/**
 * Verifies that defining an IR instruction colliding with an existing symbol in table fails.
 */
TEST_F(IrInstructionPassTest, TestInstructionCollidesWithExistingSymbolFails)
{
    // Pre-declare a type symbol with the colliding name
    registerType("ClashingName", DSL::Ast::TypeDef::TypeKind::Integer, 32, 32, 1);

    std::string source = R"(
        ir_inst ClashingName () {
            CATEGORY(System);
        };
    )";

    auto ast = parseIrInstDefFile(source);
    ASSERT_TRUE(ast.has_value());

    IrInstructionPass pass;
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));
}

// ============================================================================
// 5. Null Safety Checks
// ============================================================================

/**
 * Verifies safe handling and error recovery when IrInstructionPass receives nullptr arguments.
 */
TEST_F(IrInstructionPassTest, TestNullptrArguments)
{
    IrInstructionPass pass;
    EXPECT_FALSE(pass.run(nullptr, getSymbolTable(), nullptr));
    EXPECT_FALSE(pass.run(getDiagCollector(), nullptr, nullptr));

    DSL::Ast::IrInstDef::IrInstDefFile file;
    EXPECT_FALSE(pass.run(getDiagCollector(), nullptr, &file));
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), nullptr));
}
