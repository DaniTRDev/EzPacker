#include "EzDslTestSuite.h"
#include "Ast/LegalizeActionDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/LegalizeActionDefLang.h"
#include "Parser/ParseContext.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/LegalizeActionPass.h"

/**
 * Test fixture for semantic validation and symbol resolution of target legalization actions (LegalizeActionPass).
 */
class LegalizeActionPassTest : public DslTestSuiteAsGtest
{
  protected:
    std::unique_ptr<SymbolTable> m_table;

    /**
     * Initializes the symbol table with primitive scalar types and standard IR instructions before each test.
     */
    void SetUp() override
    {
        DslTestSuiteAsGtest::SetUp();
        m_table = std::make_unique<SymbolTable>(getAllocator());
        registerPrimitiveTypes();
        registerDefaultIrInstructions();

        m_table->enterScope("TargetScope");
        registerCustomActions();
    }

    /**
     * Helper to declare a scalar type symbol with name and bit width in the symbol table.
     */
    void registerType(std::string_view name, uint32_t bitWidth)
    {
        Sema::Symbols::TypeSymbol typeSym{ .m_name = name,
                                           .m_kind = static_cast<DSL::Ast::TypeDef::TypeKind>(0),
                                           .m_bitWidth = bitWidth };
        m_table->declareSym(nullptr, SymbolFlags::IsDefined, SymbolType::Type, typeSym, name);
    }

    void registerCustomAct(std::string_view name)
    {
        Sema::Symbols::LegalizeRewriteRuleSymbol sym{};
        m_table->declareSym(nullptr, SymbolFlags::IsReferenced, SymbolType::LegalizeRule, sym, name);
    }

    /**
     * Registers standard primitive integer and floating-point types in the mock symbol table.
     */
    void registerPrimitiveTypes()
    {
        registerType("i1", 1);
        registerType("i8", 8);
        registerType("i16", 16);
        registerType("i32", 32);
        registerType("i64", 64);
        registerType("i128", 128);
        registerType("f32", 32);
        registerType("f64", 64);
    }

    void registerCustomActions()
    {
        registerCustomAct("MyAction");
        registerCustomAct("MyAction2");
    }

    /**
     * Helper to declare a generic IR instruction symbol in the mock symbol table.
     */
    void registerIrInstruction(std::string_view name)
    {
        Sema::Symbols::IrInstructionSymbol irSym{ .m_name = name,
                                                  .m_category = static_cast<DSL::Ast::IrInstDef::IrInstCategory>(0),
                                                  .m_tier = static_cast<DSL::Ast::IrInstDef::IrInstTier>(0),
                                                  .m_flagsMask = static_cast<DSL::Ast::IrInstDef::IrInstFlag>(0),
                                                  .m_operands = std::pmr::vector<Sema::Symbols::IrOperandSymbol>{
                                                          getAllocator() } };
        m_table->declareSym(nullptr, SymbolFlags::IsDefined, SymbolType::IrInstruction, irSym, name);
    }

    /**
     * Registers standard IR instruction symbols used across legalization tests.
     */
    void registerDefaultIrInstructions()
    {
        registerIrInstruction("ADD");
        registerIrInstruction("SUB");
        registerIrInstruction("SDIV");
        registerIrInstruction("SEXT");
        registerIrInstruction("CONV");
        registerIrInstruction("CAST");
        registerIrInstruction("BITCAST");
    }

    /**
     * Parses a string containing legalization action DSL into an AST.
     */
    std::optional<DSL::Ast::LegalizeActionDef::TargetLegalizeDef> parseFile(const std::string &source)
    {
        ParseContext ctx = createParseContextFromBuff("LegalizeActionPassTest", source);
        return ctx.parse<DSL::Parser::LegalizeActionDef::TargetLegalizeDef,
                         DSL::Ast::LegalizeActionDef::TargetLegalizeDef>();
    }

    /**
     * Executes the LegalizeActionPass semantic analysis pass over the given source code string.
     */
    bool runPass(const std::string &source)
    {
        auto ast = parseFile(source);
        if (!ast.has_value())
        {
            return false;
        }
        return LegalizeActionPass::run(getDiagCollector(), m_table.get(), &ast.value());
    }
};

// ============================================================================
// 1. Success & Symbol Resolution Tests
// ============================================================================

/**
 * Verifies semantic resolution of a valid LEGAL action clause marking i32 and f32 as directly supported.
 */
TEST_F(LegalizeActionPassTest, TestValidLegalActionDeclaration)
{
    std::string code = R"(
action ADD {
    LEGAL(i32, f32);
};
)";

    ASSERT_TRUE(runPass(code));

    Symbol *sym = m_table->getSymByName("ADD");
    ASSERT_NE(sym, nullptr);

    const auto *actionData = sym->getIf<Sema::Symbols::LegalizeActionSymbol>();
    ASSERT_NE(actionData, nullptr);
    EXPECT_EQ(actionData->m_genericOpcode, "ADD");
    EXPECT_EQ(actionData->m_maxOperandIndex, 0);
    ASSERT_EQ(actionData->m_clauses.size(), 1);

    const auto &clause = actionData->m_clauses[0];
    EXPECT_EQ(clause.m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::Legal);
    ASSERT_EQ(clause.m_types.size(), 2);
    EXPECT_EQ(clause.m_types[0].m_operandIndex, std::nullopt);
    EXPECT_EQ(clause.m_types[1].m_operandIndex, std::nullopt);
}

/**
 * Verifies semantic resolution of WIDENS, NARROWS, and BITCAST clauses and resolution of target type IDs.
 */
TEST_F(LegalizeActionPassTest, TestValidWidenNarrowAndBitcastTransformations)
{
    std::string code = R"(
action CONV {
    WIDENS(i1, i8, i16) >> i32;
    NARROWS(i64, i128) >> i32;
    BITCAST(f32) >> i32;
};
)";

    ASSERT_TRUE(runPass(code));

    Symbol *sym = m_table->getSymByName("CONV");
    ASSERT_NE(sym, nullptr);

    const auto *actionData = sym->getIf<Sema::Symbols::LegalizeActionSymbol>();
    ASSERT_NE(actionData, nullptr);
    ASSERT_EQ(actionData->m_clauses.size(), 3);

    Symbol *i32Sym = m_table->getSymByName("i32");
    ASSERT_NE(i32Sym, nullptr);

    EXPECT_EQ(actionData->m_maxOperandIndex, 0);

    // Clause 1: WIDENS
    EXPECT_EQ(actionData->m_clauses[0].m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::WidenScalar);
    EXPECT_EQ(actionData->m_clauses[0].m_types.size(), 3);
    EXPECT_EQ(actionData->m_clauses[0].m_targetTypeId, i32Sym->getId());

    // Clause 2: NARROWS
    EXPECT_EQ(actionData->m_clauses[1].m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::NarrowScalar);
    EXPECT_EQ(actionData->m_clauses[1].m_types.size(), 2);
    EXPECT_EQ(actionData->m_clauses[1].m_targetTypeId, i32Sym->getId());

    // Clause 3: BITCAST
    EXPECT_EQ(actionData->m_clauses[2].m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::Bitcast);
    EXPECT_EQ(actionData->m_clauses[2].m_types.size(), 1);
    EXPECT_EQ(actionData->m_clauses[2].m_targetTypeId, i32Sym->getId());
}

/**
 * Verifies semantic resolution of a LIBCALL lowering action mapping to an external runtime library symbol string.
 */
TEST_F(LegalizeActionPassTest, TestValidLibcallAction)
{
    std::string code = R"(
action SDIV {
    LIBCALL(i64) >> "__divdi3";
};
)";

    ASSERT_TRUE(runPass(code));

    Symbol *sym = m_table->getSymByName("SDIV");
    ASSERT_NE(sym, nullptr);

    const auto *actionData = sym->getIf<Sema::Symbols::LegalizeActionSymbol>();
    ASSERT_NE(actionData, nullptr);
    ASSERT_EQ(actionData->m_clauses.size(), 1);
    EXPECT_EQ(actionData->m_maxOperandIndex, 0);

    const auto &clause = actionData->m_clauses[0];
    EXPECT_EQ(clause.m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::Libcall);
    ASSERT_TRUE(clause.m_libcallSymbol.has_value());
    EXPECT_EQ(*clause.m_libcallSymbol, "__divdi3");
    EXPECT_FALSE(clause.m_targetTypeId.has_value());
}

TEST_F(LegalizeActionPassTest, TestValidCustomAction)
{
    std::string code = R"(
action SDIV {
    CUSTOM() >> MyAction;
};
)";

    ASSERT_TRUE(runPass(code));

    Symbol *sym = m_table->getSymByName("SDIV");
    ASSERT_NE(sym, nullptr);

    const auto *actionData = sym->getIf<Sema::Symbols::LegalizeActionSymbol>();
    ASSERT_NE(actionData, nullptr);
    ASSERT_EQ(actionData->m_clauses.size(), 1);
    EXPECT_EQ(actionData->m_maxOperandIndex, 0);

    const auto &clause = actionData->m_clauses[0];
    EXPECT_EQ(clause.m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::Custom);
    ASSERT_TRUE(clause.m_customRules.has_value());

    auto rules = clause.m_customRules.value();
    ASSERT_EQ(rules.size(), 1);
}

TEST_F(LegalizeActionPassTest, TestValidCustomAction2)
{
    std::string code = R"(
action SDIV {
    CUSTOM() >> MyAction >> MyAction2;
};
)";

    ASSERT_TRUE(runPass(code));

    Symbol *sym = m_table->getSymByName("SDIV");
    ASSERT_NE(sym, nullptr);

    const auto *actionData = sym->getIf<Sema::Symbols::LegalizeActionSymbol>();
    ASSERT_NE(actionData, nullptr);
    ASSERT_EQ(actionData->m_clauses.size(), 1);
    EXPECT_EQ(actionData->m_maxOperandIndex, 0);

    const auto &clause = actionData->m_clauses[0];
    EXPECT_EQ(clause.m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::Custom);
    ASSERT_TRUE(clause.m_customRules.has_value());

    auto rules = clause.m_customRules.value();
    ASSERT_EQ(rules.size(), 2);
}

TEST_F(LegalizeActionPassTest, TestInvalidCustomAction)
{
    std::string code = R"(
action SDIV {
    CUSTOM() >> MyActionInvalid;
};
)";

    ASSERT_FALSE(runPass(code));
}

/**
 * Verifies semantic resolution of heterogeneous type constraints where specific operand indices are designated.
 */
TEST_F(LegalizeActionPassTest, TestHeterogeneousConstraintOperandIndices)
{
    std::string code = R"(
action SEXT {
    LEGAL(i64);
    WIDENS(i8:1, i16:1) >> i32;
};
)";

    ASSERT_TRUE(runPass(code));

    Symbol *sym = m_table->getSymByName("SEXT");
    ASSERT_NE(sym, nullptr);

    const auto *actionData = sym->getIf<Sema::Symbols::LegalizeActionSymbol>();
    ASSERT_NE(actionData, nullptr);
    ASSERT_EQ(actionData->m_clauses.size(), 2);
    EXPECT_EQ(actionData->m_maxOperandIndex, 1);

    const auto &widenClause = actionData->m_clauses[1];
    ASSERT_EQ(widenClause.m_types.size(), 2);
    ASSERT_TRUE(widenClause.m_types[0].m_operandIndex.has_value());
    EXPECT_EQ(*widenClause.m_types[0].m_operandIndex, 1u);
    ASSERT_TRUE(widenClause.m_types[1].m_operandIndex.has_value());
    EXPECT_EQ(*widenClause.m_types[1].m_operandIndex, 1u);
}

// ============================================================================
// 2. Opcode Validation & Semantic Error Tests
// ============================================================================

/**
 * Verifies that the semantic pass rejects action blocks for undefined generic IR opcodes.
 */
TEST_F(LegalizeActionPassTest, TestErrorUndefinedIrOpcode)
{
    std::string code = R"(
action UNKNOWN_OPCODE {
    LEGAL(i32);
};
)";

    EXPECT_FALSE(runPass(code));
}

/**
 * Verifies that the semantic pass rejects action declarations targeting symbols that are not IR instructions.
 */
TEST_F(LegalizeActionPassTest, TestErrorSymbolNotAnIrInstruction)
{
    // "i32" is defined as a SymbolType::Type, not a SymbolType::IrInstruction
    std::string code = R"(
action i32 {
    LEGAL(i32);
};
)";

    EXPECT_FALSE(runPass(code));
}

/**
 * Verifies that the semantic pass rejects clauses referencing undefined source types.
 */
TEST_F(LegalizeActionPassTest, TestErrorUndefinedSourceType)
{
    std::string code = R"(
action ADD {
    LEGAL(unknown_type);
};
)";

    EXPECT_FALSE(runPass(code));
}

/**
 * Verifies that the semantic pass rejects clauses referencing undefined target types.
 */
TEST_F(LegalizeActionPassTest, TestErrorUndefinedTargetType)
{
    std::string code = R"(
action ADD {
    WIDENS(i8) >> unknown_target;
};
)";

    EXPECT_FALSE(runPass(code));
}

/**
 * Verifies semantic validation error when WIDENS targets a narrower bit width.
 */
TEST_F(LegalizeActionPassTest, TestErrorInvalidWidenBitwidth)
{
    std::string code = R"(
action ADD {
    WIDENS(i32) >> i8;
};
)";

    EXPECT_FALSE(runPass(code));
}

/**
 * Verifies semantic validation error when NARROWS targets a wider bit width.
 */
TEST_F(LegalizeActionPassTest, TestErrorInvalidNarrowBitwidth)
{
    std::string code = R"(
action ADD {
    NARROWS(i8) >> i32;
};
)";

    EXPECT_FALSE(runPass(code));
}

/**
 * Verifies semantic validation error when BITCAST source and target types have mismatched bit widths.
 */
TEST_F(LegalizeActionPassTest, TestErrorBitcastBitwidthMismatch)
{
    std::string code = R"(
action CAST {
    BITCAST(i16) >> i32;
};
)";

    EXPECT_FALSE(runPass(code));
}

/**
 * Verifies semantic validation error on multiple action declarations for the same generic opcode.
 */
TEST_F(LegalizeActionPassTest, TestErrorOpcodeRedefinition)
{
    std::string code = R"(
action ADD {
    LEGAL(i32);
};

action ADD {
    LEGAL(i64);
};
)";

    EXPECT_FALSE(runPass(code));
}

/**
 * Verifies semantic validation error when LIBCALL erroneously specifies a target type instead of a symbol string.
 */
TEST_F(LegalizeActionPassTest, TestErrorLibcallWithTargetType)
{
    std::string code = R"(
action SDIV {
    LIBCALL(i64) >> i32;
};
)";

    EXPECT_FALSE(runPass(code));
}

/**
 * Verifies semantic validation error when a LEGAL clause specifies a transformation target type.
 */
TEST_F(LegalizeActionPassTest, TestErrorLegalClauseWithTargetType)
{
    std::string code = R"(
action ADD {
    LEGAL(i32) >> i32;
};
)";

    EXPECT_FALSE(runPass(code));
}