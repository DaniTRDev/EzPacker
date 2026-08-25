#include "EzDslTestSuite.h"
#include "Ast/InstructionDefLangAst.h"
#include "Ast/TypeDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/InstructionDefLang.h"
#include "Parser/ParseContext.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/Symbols.h"
#include "SemaPasses/TargetInstPass.h"

/**
 * Test fixture for semantic validation and encoding analysis of target instruction definitions (InstructionDefPass / TargetInstPass).
 */
class InstructionDefPassTest : public DslTestSuiteAsGtest
{
  protected:
    /**
     * Helper to parse target instruction definition DSL into an InstDefFile AST.
     */
    std::optional<DSL::Ast::InstDef::InstDefFile> parseInstDef(const std::string &source)
    {
        ParseContext ctx = createParseContextFromBuff("test.idf", source);
        return ctx.parse<DSL::Parser::InstDef::InstDefFile, DSL::Ast::InstDef::InstDefFile>();
    }

    /**
     * Initializes mock primitive types (i5, i12, i20, i32, f64) and target register classes (GPR, FPR, CSR).
     */
    void setupMockTargetEnvironment(SymbolTable &table)
    {
        // Primitive scalar types
        table.declareSym(nullptr,
                         SymbolFlags::IsDefined,
                         SymbolType::Type,
                         Sema::Symbols::TypeSymbol{ .m_name = "i5",
                                                    .m_kind = DSL::Ast::TypeDef::TypeKind::Integer,
                                                    .m_bitWidth = 5 },
                         "i5");

        table.declareSym(nullptr,
                         SymbolFlags::IsDefined,
                         SymbolType::Type,
                         Sema::Symbols::TypeSymbol{ .m_name = "i12",
                                                    .m_kind = DSL::Ast::TypeDef::TypeKind::Integer,
                                                    .m_bitWidth = 12 },
                         "i12");

        table.declareSym(nullptr,
                         SymbolFlags::IsDefined,
                         SymbolType::Type,
                         Sema::Symbols::TypeSymbol{ .m_name = "i20",
                                                    .m_kind = DSL::Ast::TypeDef::TypeKind::Integer,
                                                    .m_bitWidth = 20 },
                         "i20");

        table.declareSym(nullptr,
                         SymbolFlags::IsDefined,
                         SymbolType::Type,
                         Sema::Symbols::TypeSymbol{ .m_name = "i32",
                                                    .m_kind = DSL::Ast::TypeDef::TypeKind::Integer,
                                                    .m_bitWidth = 32 },
                         "i32");

        table.declareSym(nullptr,
                         SymbolFlags::IsDefined,
                         SymbolType::Type,
                         Sema::Symbols::TypeSymbol{ .m_name = "f64",
                                                    .m_kind = DSL::Ast::TypeDef::TypeKind::FloatingPoint,
                                                    .m_bitWidth = 64 },
                         "f64");

        // Target Hardware Register Classes
        table.declareSym(nullptr,
                         SymbolFlags::IsDefined,
                         SymbolType::RegisterClass,
                         Sema::Symbols::RegisterClassSymbol{ .m_name = "GPR" },
                         "GPR");

        table.declareSym(nullptr,
                         SymbolFlags::IsDefined,
                         SymbolType::RegisterClass,
                         Sema::Symbols::RegisterClassSymbol{ .m_name = "FPR" },
                         "FPR");

        table.declareSym(nullptr,
                         SymbolFlags::IsDefined,
                         SymbolType::RegisterClass,
                         Sema::Symbols::RegisterClassSymbol{ .m_name = "CSR" },
                         "CSR");
    }
};

// ============================================================================
// 1. Format Declarations, Slices & Default Value Expressions
// ============================================================================

/**
 * Verifies format declaration with constant folding of default value expressions (e.g. shift/and/not expressions).
 */
TEST_F(InstructionDefPassTest, TestValidFormatWithDefaultConstantFolding)
{
    std::string test = R"dsl(
format R_TYPE(32) {
    opcode[0:6] = 0x33;
    rd[7:11];
    funct3[12:14] = (1 << 2) | 1;
    rs1[15:19];
    rs2[20:24];
    funct7[25:31] = ~0x1 & 0x7F;
};
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_TRUE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));

    const Symbol *fmtSym = table.getSymByName("R_TYPE");
    ASSERT_NE(fmtSym, nullptr);
    const auto *fmtData = fmtSym->getIf<Sema::Symbols::InstructionFormatSymbol>();
    ASSERT_NE(fmtData, nullptr);
    ASSERT_EQ(fmtData->m_fields.size(), 6);

    // opcode default = 0x33
    ASSERT_TRUE(fmtData->m_fields[0].m_defaultValue.has_value());
    ASSERT_TRUE(std::holds_alternative<uint64_t>(*fmtData->m_fields[0].m_defaultValue));
    EXPECT_EQ(std::get<uint64_t>(*fmtData->m_fields[0].m_defaultValue), 0x33);

    // rd has no default
    EXPECT_FALSE(fmtData->m_fields[1].m_defaultValue.has_value());

    // funct3 default = (1 << 2) | 1 = 5
    ASSERT_TRUE(fmtData->m_fields[2].m_defaultValue.has_value());
    ASSERT_TRUE(std::holds_alternative<uint64_t>(*fmtData->m_fields[2].m_defaultValue));
    EXPECT_EQ(std::get<uint64_t>(*fmtData->m_fields[2].m_defaultValue), 5);

    // funct7 default = ~0x1 & 0x7F = 0x7E = 126
    ASSERT_TRUE(fmtData->m_fields[5].m_defaultValue.has_value());
    ASSERT_TRUE(std::holds_alternative<uint64_t>(*fmtData->m_fields[5].m_defaultValue));
    EXPECT_EQ(std::get<uint64_t>(*fmtData->m_fields[5].m_defaultValue), 0x7E);
}

/**
 * Verifies that zero-width instruction formats are rejected.
 */
TEST_F(InstructionDefPassTest, TestZeroOrExcessiveBitWidthFormatFails)
{
    std::string test = R"dsl(
format BAD_FORMAT(0) {
    opcode[0:0];
};
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that duplicate field names within a format declaration are rejected.
 */
TEST_F(InstructionDefPassTest, TestDuplicateFormatFieldNamesFail)
{
    std::string test = R"dsl(
format R_TYPE(32) {
    field[0:7];
    field[8:15];
};
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that format field slices that exceed format bit width are rejected.
 */
TEST_F(InstructionDefPassTest, TestFormatFieldSliceOutOfBoundsFails)
{
    std::string test = R"dsl(
format R_TYPE(32) {
    opcode[0:6];
    bad_field[20:32];
};
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that overlapping field slice intervals in format declarations are rejected.
 */
TEST_F(InstructionDefPassTest, TestOverlappingFormatFieldsFail)
{
    std::string test = R"dsl(
format R_TYPE(32) {
    fieldA[0:15];
    fieldB[10:20];
};
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that format default constants overflowing field bit width are rejected.
 */
TEST_F(InstructionDefPassTest, TestFormatDefaultConstantOverflowFails)
{
    std::string test = R"dsl(
format R_TYPE(32) {
    funct3[0:2] = 0xFF; // Width = 3 bits (max 7), 0xFF overflows
};
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that format field defaults cannot reference runtime identifier names.
 */
TEST_F(InstructionDefPassTest, TestFormatDefaultUsingIdentifierFails)
{
    std::string test = R"dsl(
format R_TYPE(32) {
    opcode[0:6] = rd; // Identifiers invalid in standalone format defaults
};
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

// ============================================================================
// 2. Complex Bit Expressions & Instruction Format Overrides
// ============================================================================

/**
 * Verifies nested bit expressions in instruction field assignments (shift, or, and).
 */
TEST_F(InstructionDefPassTest, TestComplexNestedBitExpressionInFormatAssignment)
{
    std::string test = R"dsl(
format R_TYPE(32) {
    opcode[0:6];
    rd[7:11];
    funct3[12:14];
    rs1[15:19];
    rs2[20:24];
    funct7[25:31];
};

inst BIT_MANIP(GPR:rd OUT, GPR:rs1 IN, GPR:rs2 IN, GPR:rs3 IN) format R_TYPE {
    FORMAT(
        opcode = 0x33,
        funct3 = 0,
        funct7 = 0,
        rd = rd,
        rs1 = rs1,
        rs2 = (rs1 << ((rs2 | rs3) & 0x1F))
    );
    ASM("bitmanip $rd, $rs1, $rs2, $rs3");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_TRUE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));

    const Symbol *instSym = table.getSymByName("BIT_MANIP");
    ASSERT_NE(instSym, nullptr);
    const auto *instData = instSym->getIf<Sema::Symbols::TargetInstructionSymbol>();
    ASSERT_NE(instData, nullptr);
    ASSERT_EQ(instData->m_fieldAssignments.size(), 6);

    // Verify rs2 expression structure: (rs1 << ((rs2 | rs3) & 0x1F))
    const auto &rs2Assign = instData->m_fieldAssignments[5];
    EXPECT_EQ(rs2Assign.m_fieldName, "rs2");
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<Sema::Symbols::ResolvedBitExpr>>(rs2Assign.m_value));

    const auto &rootExpr = std::get<std::shared_ptr<Sema::Symbols::ResolvedBitExpr>>(rs2Assign.m_value);
    EXPECT_EQ(rootExpr->m_op, DSL::Ast::InstDef::BitExprOp::Shl);

    // LHS is rs1 operand reference
    ASSERT_TRUE(std::holds_alternative<Sema::Symbols::SlicedOperandRef>(rootExpr->m_lhs));
    EXPECT_EQ(std::get<Sema::Symbols::SlicedOperandRef>(rootExpr->m_lhs).m_operandName, "rs1");

    // RHS is sub-expression ((rs2 | rs3) & 0x1F)
    ASSERT_TRUE(rootExpr->m_rhs.has_value());
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<Sema::Symbols::ResolvedBitExpr>>(*rootExpr->m_rhs));
    const auto &andExpr = std::get<std::shared_ptr<Sema::Symbols::ResolvedBitExpr>>(*rootExpr->m_rhs);
    EXPECT_EQ(andExpr->m_op, DSL::Ast::InstDef::BitExprOp::And);

    // Constant 0x1F on RHS of AND
    ASSERT_TRUE(andExpr->m_rhs.has_value());
    ASSERT_TRUE(std::holds_alternative<uint64_t>(*andExpr->m_rhs));
    EXPECT_EQ(std::get<uint64_t>(*andExpr->m_rhs), 0x1F);
}

/**
 * Verifies sliced operand mapping into split instruction fields (imm4_0 and imm11_5).
 */
TEST_F(InstructionDefPassTest, TestSlicedOperandsAndSplicedFields)
{
    std::string test = R"dsl(
format S_TYPE(32) {
    opcode[0:6];
    imm4_0[7:11];
    funct3[12:14];
    rs1[15:19];
    rs2[20:24];
    imm11_5[25:31];
};

inst SW(GPR:rs2 IN, GPR:rs1 IN, simm(i12):imm12 IN) format S_TYPE {
    FORMAT(
        opcode = 0x23,
        funct3 = 2,
        rs1 = rs1,
        rs2 = rs2,
        imm4_0 = imm12[0:4],
        imm11_5 = imm12[5:11]
    );
    ASM("sw $rs2, ${imm12}(${rs1})");
    FLAGS(mayStore);
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_TRUE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));

    const Symbol *instSym = table.getSymByName("SW");
    ASSERT_NE(instSym, nullptr);
    const auto *instData = instSym->getIf<Sema::Symbols::TargetInstructionSymbol>();
    ASSERT_NE(instData, nullptr);

    // Verify imm4_0 sliced assignment
    const auto &imm4Assign = instData->m_fieldAssignments[4];
    EXPECT_EQ(imm4Assign.m_fieldName, "imm4_0");
    ASSERT_TRUE(std::holds_alternative<Sema::Symbols::SlicedOperandRef>(imm4Assign.m_value));
    const auto &sliceLow = std::get<Sema::Symbols::SlicedOperandRef>(imm4Assign.m_value);
    EXPECT_EQ(sliceLow.m_operandName, "imm12");
    EXPECT_EQ(sliceLow.m_slice.m_from, 0);
    EXPECT_EQ(sliceLow.m_slice.m_to, 4);

    // Verify imm11_5 sliced assignment
    const auto &imm11Assign = instData->m_fieldAssignments[5];
    EXPECT_EQ(imm11Assign.m_fieldName, "imm11_5");
    ASSERT_TRUE(std::holds_alternative<Sema::Symbols::SlicedOperandRef>(imm11Assign.m_value));
    const auto &sliceHigh = std::get<Sema::Symbols::SlicedOperandRef>(imm11Assign.m_value);
    EXPECT_EQ(sliceHigh.m_operandName, "imm12");
    EXPECT_EQ(sliceHigh.m_slice.m_from, 5);
    EXPECT_EQ(sliceHigh.m_slice.m_to, 11);
}

/**
 * Verifies that field slice assignments exceeding destination field width fail.
 */
TEST_F(InstructionDefPassTest, TestLhsFieldSliceExceedsFieldWidthFails)
{
    std::string test = R"dsl(
format R_TYPE(32) {
    opcode[0:6];
};

inst BAD(GPR:rd OUT) format R_TYPE {
    FORMAT(
        opcode[0:8] = 0x33
    );
    ASM("bad");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that operand slice references exceeding operand bit width fail.
 */
TEST_F(InstructionDefPassTest, TestRhsSliceExceedsOperandWidthFails)
{
    std::string test = R"dsl(
format I_TYPE(32) {
    imm[0:19];
};

inst BAD(simm(i12):imm12 IN) format I_TYPE {
    FORMAT(
        imm = imm12[0:15] // imm12 is only 12 bits wide (0..11)
    );
    ASM("bad");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that overlapping field assignments in FORMAT(...) fail.
 */
TEST_F(InstructionDefPassTest, TestOverlappingFieldAssignmentsInInstructionFails)
{
    std::string test = R"dsl(
format R_TYPE(32) {
    payload[0:15];
};

inst BAD(GPR:r1 IN, GPR:r2 IN) format R_TYPE {
    FORMAT(
        payload[0:7] = r1,
        payload[6:12] = r2 // Overlaps at bits 6..7
    );
    ASM("bad");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that assigning to a nonexistent format field fails.
 */
TEST_F(InstructionDefPassTest, TestAssigningNonExistentFormatFieldFails)
{
    std::string test = R"dsl(
format R_TYPE(32) {
    opcode[0:6];
};

inst BAD(GPR:rd OUT) format R_TYPE {
    FORMAT(
        opcode = 0x33,
        non_existent_field = 0
    );
    ASM("bad");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that referencing an undeclared operand in FORMAT(...) fails.
 */
TEST_F(InstructionDefPassTest, TestRhsRefersToUnknownOperandFails)
{
    std::string test = R"dsl(
format R_TYPE(32) {
    opcode[0:6];
    rd[7:11];
};

inst BAD(GPR:rd OUT) format R_TYPE {
    FORMAT(
        opcode = 0x33,
        rd = undeclared_operand
    );
    ASM("bad");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that numeric constant values overflowing the destination field slice width fail.
 */
TEST_F(InstructionDefPassTest, TestConstantValueOverflowsTargetFieldSliceFails)
{
    std::string test = R"dsl(
format R_TYPE(32) {
    funct3[0:2]; // Width = 3 bits
};

inst BAD() format R_TYPE {
    FORMAT(
        funct3 = 0x10 // 16 does not fit in 3 bits (max 7)
    );
    ASM("bad");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

// ============================================================================
// 3. Operand Types, Slices, Immediates & Implicits
// ============================================================================

/**
 * Verifies instructions with unparameterized immediate operands.
 */
TEST_F(InstructionDefPassTest, TestMixedRegistersAndUnparameterizedImmediates)
{
    std::string test = R"dsl(
format SHIFT_TYPE(32) {
    opcode[0:6];
    rd[7:11];
    shamt[12:16];
    rs1[17:21];
};

inst SLLI(GPR:rd OUT, GPR:rs1 IN, imm:shamt IN) format SHIFT_TYPE {
    FORMAT(
        opcode = 0x13,
        rd = rd,
        rs1 = rs1,
        shamt = shamt
    );
    ASM("slli $rd, $rs1, $shamt");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_TRUE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));

    const Symbol *instSym = table.getSymByName("SLLI");
    ASSERT_NE(instSym, nullptr);
    const auto *instData = instSym->getIf<Sema::Symbols::TargetInstructionSymbol>();
    ASSERT_NE(instData, nullptr);

    ASSERT_EQ(instData->m_args.size(), 3);
    EXPECT_EQ(instData->m_args[2].m_name, "shamt");
    EXPECT_EQ(instData->m_args[2].m_kind, DSL::Ast::InstDef::InstOperandKind::Immediate);
    EXPECT_EQ(instData->m_args[2].m_typeOrClassId, InvalidSymbolId); // Unparameterized
}

/**
 * Verifies implicit argument resolution with distinct directions (OUT and INOUT).
 */
TEST_F(InstructionDefPassTest, TestMultipleImplicitArgumentsInOut)
{
    std::string test = R"dsl(
format SYS_TYPE(32) {
    f[0:31];
};

inst CALL_SYS(GPR:target IN) format SYS_TYPE {
    IMPLICIT(GPR:ra OUT, CSR:fcsr INOUT);
    ASM("call $target");
    LATENCY(3);
    FLAGS(isCall, isTerminator);
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_TRUE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));

    const Symbol *instSym = table.getSymByName("CALL_SYS");
    ASSERT_NE(instSym, nullptr);
    const auto *instData = instSym->getIf<Sema::Symbols::TargetInstructionSymbol>();
    ASSERT_NE(instData, nullptr);

    ASSERT_EQ(instData->m_implicitArgs.size(), 2);
    EXPECT_EQ(instData->m_implicitArgs[0].m_name, "ra");
    EXPECT_EQ(instData->m_implicitArgs[0].m_dir, DSL::Ast::InstDef::InstOperandDir::ArgOut);
    EXPECT_EQ(instData->m_implicitArgs[1].m_name, "fcsr");
    EXPECT_EQ(instData->m_implicitArgs[1].m_dir, DSL::Ast::InstDef::InstOperandDir::ArgInOut);
}

/**
 * Verifies that operand name collisions between explicit and implicit operand lists fail.
 */
TEST_F(InstructionDefPassTest, TestDuplicateOperandAcrossExplicitAndImplicitFails)
{
    std::string test = R"dsl(
format F(32) { f[0:31]; };

inst BAD(GPR:rd OUT) format F {
    IMPLICIT(GPR:rd IN); // Collides with explicit operand 'rd'
    ASM("bad");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that immediate operands declared with OUT direction fail.
 */
TEST_F(InstructionDefPassTest, TestImmediateAsOutputFails)
{
    std::string test = R"dsl(
format F(32) { f[0:31]; };
inst BAD(simm(i12):imm OUT) format F {
    ASM("bad");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that immediate operands declared with INOUT direction fail.
 */
TEST_F(InstructionDefPassTest, TestImmediateAsInOutFails)
{
    std::string test = R"dsl(
format F(32) { f[0:31]; };
inst BAD(imm:val INOUT) format F {
    ASM("bad");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that operands referencing nonexistent register classes fail.
 */
TEST_F(InstructionDefPassTest, TestUndefinedRegisterClassFails)
{
    std::string test = R"dsl(
format F(32) { f[0:31]; };
inst BAD(NON_EXISTENT_CLASS:rd OUT) format F {
    ASM("bad");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that immediate operands referencing undefined type parameters fail.
 */
TEST_F(InstructionDefPassTest, TestUndefinedImmediateTypeParamFails)
{
    std::string test = R"dsl(
format F(32) { f[0:31]; };
inst BAD(simm(unknown_type):imm IN) format F {
    ASM("bad");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

// ============================================================================
// 4. Control Flow, Terminators & Instruction Flags
// ============================================================================

/**
 * Verifies branch instructions requiring both isBranch and isTerminator flags.
 */
TEST_F(InstructionDefPassTest, TestValidBranchWithTerminator)
{
    std::string test = R"dsl(
format B_TYPE(32) { f[0:31]; };

inst BEQ(GPR:rs1 IN, GPR:rs2 IN, simm(i12):offset IN) format B_TYPE {
    ASM("beq $rs1, $rs2, $offset");
    FLAGS(isBranch, isTerminator);
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_TRUE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));

    const Symbol *instSym = table.getSymByName("BEQ");
    ASSERT_NE(instSym, nullptr);
    const auto *instData = instSym->getIf<Sema::Symbols::TargetInstructionSymbol>();
    ASSERT_NE(instData, nullptr);
    EXPECT_TRUE(instData->hasFlag(DSL::Ast::InstDef::InstFlag::IsBranch));
    EXPECT_TRUE(instData->hasFlag(DSL::Ast::InstDef::InstFlag::IsTerminator));
}

/**
 * Verifies return instructions requiring both isReturn and isTerminator flags.
 */
TEST_F(InstructionDefPassTest, TestValidReturnWithTerminator)
{
    std::string test = R"dsl(
format I_TYPE(32) { f[0:31]; };

inst JALR_RET(GPR:ra IN) format I_TYPE {
    ASM("ret");
    FLAGS(isReturn, isTerminator);
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_TRUE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));

    const Symbol *instSym = table.getSymByName("JALR_RET");
    ASSERT_NE(instSym, nullptr);
    const auto *instData = instSym->getIf<Sema::Symbols::TargetInstructionSymbol>();
    ASSERT_NE(instData, nullptr);
    EXPECT_TRUE(instData->hasFlag(DSL::Ast::InstDef::InstFlag::IsReturn));
    EXPECT_TRUE(instData->hasFlag(DSL::Ast::InstDef::InstFlag::IsTerminator));
}

/**
 * Verifies that branch instructions missing isTerminator flag fail.
 */
TEST_F(InstructionDefPassTest, TestBranchWithoutTerminatorFails)
{
    std::string test = R"dsl(
format J_TYPE(32) { f[0:31]; };

inst JMP(GPR:target IN) format J_TYPE {
    FLAGS(isBranch); // Missing isTerminator
    ASM("jmp $target");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that return instructions missing isTerminator flag fail.
 */
TEST_F(InstructionDefPassTest, TestReturnWithoutTerminatorFails)
{
    std::string test = R"dsl(
format I_TYPE(32) { f[0:31]; };

inst RET(GPR:val IN) format I_TYPE {
    FLAGS(isReturn); // Missing isTerminator
    ASM("ret");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that conflicting control flow flags (isCall, isReturn) on the same instruction fail.
 */
TEST_F(InstructionDefPassTest, TestMutuallyExclusiveControlFlowFlagsFail)
{
    std::string test = R"dsl(
format J_TYPE(32) { f[0:31]; };

inst BAD_CALL(GPR:target IN) format J_TYPE {
    FLAGS(isCall, isReturn, isTerminator); // isCall and isReturn conflict
    ASM("bad");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

// ============================================================================
// 5. Assembly Template (${symbol}) Interpolation Validation
// ============================================================================

/**
 * Verifies assembly template interpolation using ${rd}, ${rs1}, ${rs2} format.
 */
TEST_F(InstructionDefPassTest, TestValidAssemblyTemplateInterpolation)
{
    std::string test = R"dsl(
format R_TYPE(32) {
    opcode[0:6];
    rd[7:11];
    rs1[15:19];
    rs2[20:24];
};

inst ADD(GPR:rd OUT, GPR:rs1 IN, GPR:rs2 IN) format R_TYPE {
    ASM("add ${rd}, ${rs1}, ${rs2}");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_TRUE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));

    const Symbol *instSym = table.getSymByName("ADD");
    ASSERT_NE(instSym, nullptr);
    const auto *instData = instSym->getIf<Sema::Symbols::TargetInstructionSymbol>();
    ASSERT_NE(instData, nullptr);
    EXPECT_EQ(instData->m_asmTemplate, "add ${rd}, ${rs1}, ${rs2}");
}

/**
 * Verifies assembly template interpolation including implicit operands.
 */
TEST_F(InstructionDefPassTest, TestValidAssemblyTemplateWithImplicitOperands)
{
    std::string test = R"dsl(
format I_TYPE(32) {
    f[0:31];
};

inst LOAD_CUSTOM(GPR:rd OUT, GPR:rs1 IN, simm(i12):imm12 IN) format I_TYPE {
    IMPLICIT(CSR:fcsr IN);
    ASM("load_c ${rd}, ${imm12}(${rs1}) [fcsr: ${fcsr}]");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_TRUE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));

    const Symbol *instSym = table.getSymByName("LOAD_CUSTOM");
    ASSERT_NE(instSym, nullptr);
    const auto *instData = instSym->getIf<Sema::Symbols::TargetInstructionSymbol>();
    ASSERT_NE(instData, nullptr);
    EXPECT_EQ(instData->m_asmTemplate, "load_c ${rd}, ${imm12}(${rs1}) [fcsr: ${fcsr}]");
}

/**
 * Verifies assembly template without variable interpolation (literal string).
 */
TEST_F(InstructionDefPassTest, TestAssemblyTemplateWithoutInterpolationPasses)
{
    std::string test = R"dsl(
format NOP_TYPE(32) { f[0:31]; };

inst NOP() format NOP_TYPE {
    ASM("nop");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_TRUE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that assembly templates referencing undeclared operands fail.
 */
TEST_F(InstructionDefPassTest, TestAssemblyTemplateUnknownOperandFails)
{
    std::string test = R"dsl(
format R_TYPE(32) { f[0:31]; };

inst ADD(GPR:rd OUT, GPR:rs1 IN, GPR:rs2 IN) format R_TYPE {
    ASM("add ${rd}, ${rs1}, ${undeclared_operand}");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that assembly templates with unclosed braces fail.
 */
TEST_F(InstructionDefPassTest, TestAssemblyTemplateUnclosedBraceFails)
{
    std::string test = R"dsl(
format R_TYPE(32) { f[0:31]; };

inst ADD(GPR:rd OUT, GPR:rs1 IN, GPR:rs2 IN) format R_TYPE {
    ASM("add ${rd, ${rs1}, ${rs2}");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that assembly templates with empty variable references (${}) fail.
 */
TEST_F(InstructionDefPassTest, TestAssemblyTemplateEmptyVariableReferenceFails)
{
    std::string test = R"dsl(
format R_TYPE(32) { f[0:31]; };

inst ADD(GPR:rd OUT, GPR:rs1 IN, GPR:rs2 IN) format R_TYPE {
    ASM("add ${}, ${rs1}, ${rs2}");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

// ============================================================================
// 6. Semantic Validation & Error Invariant Tests
// ============================================================================

/**
 * Verifies that instructions referencing undefined format names fail.
 */
TEST_F(InstructionDefPassTest, TestUndefinedFormatReferenceFails)
{
    std::string test = R"dsl(
inst BAD(GPR:rd OUT) format UNDEFINED_FORMAT {
    ASM("bad");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that duplicate format declarations fail.
 */
TEST_F(InstructionDefPassTest, TestDuplicateFormatDeclarationsFail)
{
    std::string test = R"dsl(
format F(32) { f[0:31]; };
format F(32) { f[0:31]; };
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that duplicate instruction declarations fail.
 */
TEST_F(InstructionDefPassTest, TestDuplicateInstructionDeclarationsFail)
{
    std::string test = R"dsl(
format F(32) { f[0:31]; };
inst NOP(GPR:rd IN) format F {
    ASM("nop");
}
inst NOP(GPR:rd IN) format F {
    ASM("nop");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that declaring an instruction whose name collides with a pre-existing root symbol fails.
 */
TEST_F(InstructionDefPassTest, TestPreExistingSymbolCollisionFails)
{
    std::string test = R"dsl(
format F(32) { f[0:31]; };
inst COLLISION(GPR:rd IN) format F {
    ASM("collision");
}
)dsl";

    auto ast = parseInstDef(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    setupMockTargetEnvironment(table);

    // Collides with existing Type symbol in root scope
    table.declareSym(nullptr, SymbolFlags::IsDefined, SymbolType::Type, Sema::Symbols::TypeSymbol{}, "COLLISION");

    EXPECT_FALSE(InstructionDefPass::run(getDiagCollector(), &table, &*ast));
}