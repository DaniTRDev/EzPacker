#include "EzDslTestSuite.h"
#include "Ast/IrInstructionDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/IrInstructionDefLang.h"
#include "Parser/ParseContext.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/Symbols.h"
#include "SemaPasses/IrInstructionPass.h"

/**
 * Test fixture for semantic validation and symbol registration pass over IR instruction definitions (IrInstructionPass).
 */
class IrInstructionPassTest : public DslTestSuiteAsGtest
{
  protected:
    /**
     * Helper to parse an IR instruction definition DSL string into an AST representation.
     */
    std::optional<DSL::Ast::IrInstDef::IrInstDefFile> parseFile(const std::string &source)
    {
        ParseContext ctx = createParseContextFromBuff("test.iid", source);
        return ctx.parse<DSL::Parser::IrInstDef::IrInstDefFile, DSL::Ast::IrInstDef::IrInstDefFile>();
    }
};

// ============================================================================
// 1. Happy Path & Symbol Table Registration Tests
// ============================================================================

/**
 * Verifies semantic validation and symbol table entry creation for a valid binary arithmetic instruction (ADD).
 */
TEST_F(IrInstructionPassTest, TestValidInstructionRegistration)
{
    std::string test = R"dsl(
ir_inst ADD(Register:dst OUT, Register:lhs IN, RegImm:rhs IN) {
    CATEGORY(Arithmetic);
    TIER(HighLevel);
    FLAGS(SizeMatch, IsCommutative);
}
)dsl";

    auto ast = parseFile(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    IrInstructionPass pass;

    EXPECT_TRUE(pass.run(getDiagCollector(), &table, &*ast));

    const Symbol *sym = table.getSymByName("ADD");
    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->getType(), SymbolType::IrInstruction);
    EXPECT_TRUE(sym->hasFlag(SymbolFlags::IsDefined));

    const auto *data = sym->getIf<Sema::Symbols::IrInstructionSymbol>();
    ASSERT_NE(data, nullptr);
    EXPECT_EQ(data->m_category, DSL::Ast::IrInstDef::IrInstCategory::Arithmetic);
    EXPECT_EQ(data->m_tier, DSL::Ast::IrInstDef::IrInstTier::HighLevel);
    EXPECT_TRUE(data->hasFlag(DSL::Ast::IrInstDef::IrInstFlag::SizeMatch));
    EXPECT_TRUE(data->hasFlag(DSL::Ast::IrInstDef::IrInstFlag::IsCommutative));
    ASSERT_EQ(data->m_operands.size(), 3);
    EXPECT_EQ(data->m_operands[0].m_name, "dst");
    EXPECT_EQ(data->m_operands[0].m_dir, DSL::Ast::IrInstDef::IrOperandDir::ArgOut);
}

/**
 * Verifies semantic processing and symbol table registration for a diverse set of instructions across all standard categories.
 */
TEST_F(IrInstructionPassTest, TestComprehensiveInstructionSet)
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

ir_inst STORE(AddressSource:dst IN, AnyValue:src IN) {
    CATEGORY(Memory);
    TIER(HighLevel);
    FLAGS(WritesMemory, HasSideEffect);
}

ir_inst JMP(Reference:target IN) {
    CATEGORY(ControlFlow);
    TIER(HighLevel);
    FLAGS(IsTerminator, IsBranch);
}

ir_inst RET(AnyValue:val IN) {
    CATEGORY(ControlFlow);
    TIER(HighLevel);
    FLAGS(IsTerminator, IsReturn, HasSideEffect);
}

ir_inst SEXT(Register:dst OUT, Register:src IN) {
    CATEGORY(Casting);
    TIER(HighLevel);
    FLAGS(DestLarger);
}

ir_inst NOP() {
    CATEGORY(System);
    TIER(HighLevel);
}
)dsl";

    auto ast = parseFile(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    IrInstructionPass pass;

    EXPECT_TRUE(pass.run(getDiagCollector(), &table, &*ast));

    EXPECT_NE(table.getSymByName("MOV"), nullptr);
    EXPECT_NE(table.getSymByName("LOAD"), nullptr);
    EXPECT_NE(table.getSymByName("STORE"), nullptr);
    EXPECT_NE(table.getSymByName("JMP"), nullptr);
    EXPECT_NE(table.getSymByName("RET"), nullptr);
    EXPECT_NE(table.getSymByName("SEXT"), nullptr);
    EXPECT_NE(table.getSymByName("NOP"), nullptr);
}

// ============================================================================
// 2. Operand Semantic Invariant Tests
// ============================================================================

/**
 * Verifies that the semantic pass rejects instructions with duplicate operand names.
 */
TEST_F(IrInstructionPassTest, TestDuplicateOperandNamesFail)
{
    std::string test = R"dsl(
ir_inst BAD(Register:dst OUT, Register:dst IN) {
    CATEGORY(Arithmetic);
    TIER(HighLevel);
}
)dsl";

    auto ast = parseFile(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    IrInstructionPass pass;

    EXPECT_FALSE(pass.run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that the semantic pass rejects immediate operands marked with output direction.
 */
TEST_F(IrInstructionPassTest, TestImmediateAsOutputFails)
{
    std::string test = R"dsl(
ir_inst BAD(Immediate:imm OUT) {
    CATEGORY(Arithmetic);
    TIER(HighLevel);
}
)dsl";

    auto ast = parseFile(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    IrInstructionPass pass;

    EXPECT_FALSE(pass.run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that the semantic pass rejects reference operands marked with INOUT direction.
 */
TEST_F(IrInstructionPassTest, TestReferenceAsInOutFails)
{
    std::string test = R"dsl(
ir_inst BAD(Reference:target INOUT) {
    CATEGORY(ControlFlow);
    TIER(HighLevel);
    FLAGS(IsTerminator, IsBranch);
}
)dsl";

    auto ast = parseFile(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    IrInstructionPass pass;

    EXPECT_FALSE(pass.run(getDiagCollector(), &table, &*ast));
}

// ============================================================================
// 3. Control Flow & Terminator Invariant Tests
// ============================================================================

/**
 * Verifies that the semantic pass enforces that instructions flagged with IsBranch must also specify IsTerminator.
 */
TEST_F(IrInstructionPassTest, TestBranchWithoutTerminatorFails)
{
    std::string test = R"dsl(
ir_inst BAD_JMP(Reference:target IN) {
    CATEGORY(ControlFlow);
    TIER(HighLevel);
    FLAGS(IsBranch);
}
)dsl";

    auto ast = parseFile(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    IrInstructionPass pass;

    EXPECT_FALSE(pass.run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that the semantic pass enforces that instructions flagged with IsReturn must also specify IsTerminator.
 */
TEST_F(IrInstructionPassTest, TestReturnWithoutTerminatorFails)
{
    std::string test = R"dsl(
ir_inst BAD_RET(AnyValue:val IN) {
    CATEGORY(ControlFlow);
    TIER(HighLevel);
    FLAGS(IsReturn);
}
)dsl";

    auto ast = parseFile(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    IrInstructionPass pass;

    EXPECT_FALSE(pass.run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that the semantic pass rejects mutually exclusive control flow flags (e.g. IsCall and IsReturn together).
 */
TEST_F(IrInstructionPassTest, TestMutuallyExclusiveControlFlowFlagsFail)
{
    std::string test = R"dsl(
ir_inst BAD_CALL(Register:dst OUT, Reference:target IN) {
    CATEGORY(ControlFlow);
    TIER(HighLevel);
    FLAGS(IsCall, IsReturn, IsTerminator);
}
)dsl";

    auto ast = parseFile(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    IrInstructionPass pass;

    EXPECT_FALSE(pass.run(getDiagCollector(), &table, &*ast));
}

// ============================================================================
// 4. Arithmetic, Size Constraint & Casting Tests
// ============================================================================

/**
 * Verifies that the semantic pass rejects IsCommutative on instructions with fewer than two inputs.
 */
TEST_F(IrInstructionPassTest, TestCommutativeWithLessThanTwoInputsFails)
{
    std::string test = R"dsl(
ir_inst NEG(Register:dst OUT, Register:src IN) {
    CATEGORY(Arithmetic);
    TIER(HighLevel);
    FLAGS(IsCommutative);
}
)dsl";

    auto ast = parseFile(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    IrInstructionPass pass;

    EXPECT_FALSE(pass.run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that the semantic pass rejects conflicting size flags (e.g. DestLarger and DestSmaller simultaneously).
 */
TEST_F(IrInstructionPassTest, TestConflictingSizeFlagsFail)
{
    std::string test = R"dsl(
ir_inst BAD_CAST(Register:dst OUT, Register:src IN) {
    CATEGORY(Casting);
    TIER(HighLevel);
    FLAGS(DestLarger, DestSmaller);
}
)dsl";

    auto ast = parseFile(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    IrInstructionPass pass;

    EXPECT_FALSE(pass.run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that casting instructions require at least one input and one output operand.
 */
TEST_F(IrInstructionPassTest, TestCastingWithoutInputOrOutputFails)
{
    std::string test = R"dsl(
ir_inst BAD_TRUNC(Register:dst OUT) {
    CATEGORY(Casting);
    TIER(HighLevel);
    FLAGS(DestSmaller);
}
)dsl";

    auto ast = parseFile(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    IrInstructionPass pass;

    EXPECT_FALSE(pass.run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that arithmetic compute instructions without side effects must define an output operand.
 */
TEST_F(IrInstructionPassTest, TestPureComputeWithoutOutputFails)
{
    std::string test = R"dsl(
ir_inst DEAD_ADD(Register:lhs IN, Register:rhs IN) {
    CATEGORY(Arithmetic);
    TIER(HighLevel);
}
)dsl";

    auto ast = parseFile(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    IrInstructionPass pass;

    EXPECT_FALSE(pass.run(getDiagCollector(), &table, &*ast));
}

// ============================================================================
// 5. Symbol Name Collision Tests
// ============================================================================

/**
 * Verifies that defining duplicate instruction symbols in the same translation unit is rejected.
 */
TEST_F(IrInstructionPassTest, TestDuplicateInstructionSymbolsInSameFileFail)
{
    std::string test = R"dsl(
ir_inst DUP() {
    CATEGORY(System);
    TIER(HighLevel);
}

ir_inst DUP() {
    CATEGORY(System);
    TIER(HighLevel);
}
)dsl";

    auto ast = parseFile(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    IrInstructionPass pass;

    EXPECT_FALSE(pass.run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that defining an IR instruction whose name collides with an existing symbol in the table fails.
 */
TEST_F(IrInstructionPassTest, TestPreExistingSymbolCollisionFails)
{
    std::string test = R"dsl(
ir_inst COLLISION() {
    CATEGORY(System);
    TIER(HighLevel);
}
)dsl";

    auto ast = parseFile(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());

    table.declareSym(nullptr,
                     SymbolFlags::IsDefined,
                     SymbolType::IrInstruction,
                     Sema::Symbols::IrInstructionSymbol{},
                     "COLLISION");

    IrInstructionPass pass;
    EXPECT_FALSE(pass.run(getDiagCollector(), &table, &*ast));
}