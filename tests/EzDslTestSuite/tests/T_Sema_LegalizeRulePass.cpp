#include "EzDslTestSuite.h"
#include "Ast/LegalizeRuleDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/LegalizeRuleDefLang.h"
#include "Parser/ParseContext.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/LegalizeRulePass.h"

/**
 * Test fixture for semantic validation and variable binding analysis of target legalization rewrite rules (LegalizeRulePass).
 */
class LegalizeRulePassTest : public DslTestSuiteAsGtest
{
  protected:
    std::unique_ptr<SymbolTable> m_table;

    /**
     * Initializes the symbol table with primitive types and generic IR instructions before each test.
     */
    void SetUp() override
    {
        DslTestSuiteAsGtest::SetUp();
        m_table = std::make_unique<SymbolTable>(getAllocator());

        registerPrimitiveTypes();
        registerDefaultIrInstructions();

        m_table->enterScope("TargetScope");
    }

    /**
     * Helper to declare a scalar type symbol with name and bit width in the mock symbol table.
     */
    void registerType(std::string_view name, uint32_t bitWidth)
    {
        Sema::Symbols::TypeSymbol typeSym{ .m_name = name,
                                           .m_kind = static_cast<DSL::Ast::TypeDef::TypeKind>(0),
                                           .m_bitWidth = bitWidth };
        m_table->declareSym(nullptr, SymbolFlags::IsDefined, SymbolType::Type, typeSym, name);
    }

    /**
     * Registers standard primitive types in the symbol table.
     */
    void registerPrimitiveTypes()
    {
        registerType("i1", 1);
        registerType("i8", 8);
        registerType("i16", 16);
        registerType("i32", 32);
        registerType("i64", 64);
        registerType("f32", 32);
        registerType("f64", 64);
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
     * Registers default IR instructions required by rewrite rule tests.
     */
    void registerDefaultIrInstructions()
    {
        registerIrInstruction("ADD");
        registerIrInstruction("SUB");
        registerIrInstruction("MUL");
        registerIrInstruction("SHL");
        registerIrInstruction("ASHR");
        registerIrInstruction("COPY");
        registerIrInstruction("SEXT");
    }

    /**
     * Parses a string containing rewrite rule DSL into an AST.
     */
    std::optional<DSL::Ast::LegalizeRuleDef::TargetLegalizeRuleDef> parseFile(const std::string &source)
    {
        ParseContext ctx = createParseContextFromBuff("LegalizeRulePassTest", source);
        return ctx.parse<DSL::Parser::LegalizeRuleDef::TargetLegalizeRuleDef,
                         DSL::Ast::LegalizeRuleDef::TargetLegalizeRuleDef>();
    }

    /**
     * Executes the LegalizeRulePass semantic analysis pass over the given source code string.
     */
    bool runPass(const std::string &source)
    {
        auto ast = parseFile(source);
        if (!ast.has_value())
        {
            return false;
        }
        return LegalizeRulePass::run(getDiagCollector(), m_table.get(), &ast.value());
    }
};

// ============================================================================
// 1. Success & Symbol Resolution Tests
// ============================================================================

/**
 * Verifies semantic resolution of a basic rewrite rule (AddZeroToCopy), checking match pattern operands,
 * literal constants, and expand sequence operand bindings.
 */
TEST_F(LegalizeRulePassTest, TestValidBasicRewriteRule)
{
    std::string code = R"(
rule AddZeroToCopy {
    match {
        ADD i32:$dst, i32:$src, 0;
    };
    expand {
        COPY $dst, $src;
    };
};
)";

    ASSERT_TRUE(runPass(code));

    Symbol *sym = m_table->getSymByName("AddZeroToCopy");
    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->getType(), SymbolType::ISelPattern);

    const auto *ruleData = sym->getIf<Sema::Symbols::LegalizeRewriteRuleSymbol>();
    ASSERT_NE(ruleData, nullptr);
    EXPECT_EQ(ruleData->m_ruleName, "AddZeroToCopy");
    ASSERT_EQ(ruleData->m_matchPatterns.size(), 1);
    ASSERT_EQ(ruleData->m_expansionSequence.size(), 1);

    // Verify Match Pattern
    const auto &matchInst = ruleData->m_matchPatterns[0];
    EXPECT_EQ(matchInst.m_opcode, "ADD");
    ASSERT_EQ(matchInst.m_operands.size(), 3);
    EXPECT_EQ(matchInst.m_operands[0].m_name, "dst");
    EXPECT_TRUE(matchInst.m_operands[0].m_typeOrClassId.has_value());
    EXPECT_EQ(matchInst.m_operands[2].m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateLiteral);
    EXPECT_EQ(matchInst.m_operands[2].m_immLiteral, 0);

    // Verify Expand Sequence
    const auto &expandInst = ruleData->m_expansionSequence[0];
    EXPECT_EQ(expandInst.m_opcode, "COPY");
    ASSERT_EQ(expandInst.m_operands.size(), 2);
    EXPECT_EQ(expandInst.m_operands[0].m_name, "dst");
    EXPECT_EQ(expandInst.m_operands[1].m_name, "src");
}

/**
 * Verifies semantic resolution of a rewrite rule with predicate guards and custom transformation functions (MulPowerOfTwoToShl).
 */
TEST_F(LegalizeRulePassTest, TestValidRuleWithPredicateAndCustomTransform)
{
    std::string code = R"(
rule MulPowerOfTwoToShl {
    match {
        MUL i32:$dst, i32:$src, imm:$c;
    };
    when {
        isPowerOfTwo($c);
    };
    expand {
        SHL $dst, $src, log2($c);
    };
};
)";

    ASSERT_TRUE(runPass(code));

    Symbol *sym = m_table->getSymByName("MulPowerOfTwoToShl");
    ASSERT_NE(sym, nullptr);

    const auto *ruleData = sym->getIf<Sema::Symbols::LegalizeRewriteRuleSymbol>();
    ASSERT_NE(ruleData, nullptr);
    ASSERT_EQ(ruleData->m_matchPatterns.size(), 1);
    ASSERT_EQ(ruleData->m_expansionSequence.size(), 1);

    const auto &expandInst = ruleData->m_expansionSequence[0];
    EXPECT_EQ(expandInst.m_opcode, "SHL");
    ASSERT_EQ(expandInst.m_operands.size(), 3);
    EXPECT_EQ(expandInst.m_operands[2].m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::CustomTransform);
    EXPECT_EQ(expandInst.m_operands[2].m_name, "log2");
}

// ============================================================================
// 2. Semantic Error & Validation Failure Tests
// ============================================================================

/**
 * Verifies that the semantic pass rejects rewrite rules with undefined opcodes in match blocks.
 */
TEST_F(LegalizeRulePassTest, TestErrorUndefinedOpcodeInMatch)
{
    std::string code = R"(
rule InvalidMatchOpcode {
    match {
        UNKNOWN_OP i32:$dst, i32:$src;
    };
    expand {
        COPY $dst, $src;
    };
};
)";

    EXPECT_FALSE(runPass(code));
}

/**
 * Verifies that the semantic pass rejects rewrite rules with undefined opcodes in expand blocks.
 */
TEST_F(LegalizeRulePassTest, TestErrorUndefinedOpcodeInExpand)
{
    std::string code = R"(
rule InvalidExpandOpcode {
    match {
        ADD i32:$dst, i32:$src, 0;
    };
    expand {
        UNKNOWN_EXPAND_OP $dst, $src;
    };
};
)";

    EXPECT_FALSE(runPass(code));
}

/**
 * Verifies that the semantic pass rejects SSA operands with undefined types.
 */
TEST_F(LegalizeRulePassTest, TestErrorUndefinedTypeInSsaOperand)
{
    std::string code = R"(
rule UnknownTypeOnOperand {
    match {
        ADD unknown_type:$dst, i32:$src, 0;
    };
    expand {
        COPY $dst, $src;
    };
};
)";

    EXPECT_FALSE(runPass(code));
}

/**
 * Verifies that the semantic pass rejects when predicate guards referencing unbound SSA variables.
 */
TEST_F(LegalizeRulePassTest, TestErrorUndefinedSsaVariableInPredicate)
{
    std::string code = R"(
rule UndefinedPredVar {
    match {
        MUL i32:$dst, i32:$src, 4;
    };
    when {
        isPowerOfTwo($unbound_var);
    };
    expand {
        SHL $dst, $src, 2;
    };
};
)";

    EXPECT_FALSE(runPass(code));
}

/**
 * Verifies that the semantic pass rejects custom transform functions referencing unbound variables.
 */
TEST_F(LegalizeRulePassTest, TestErrorUndefinedSsaVariableInCustomTransform)
{
    std::string code = R"(
rule UndefinedTransformVar {
    match {
        MUL i32:$dst, i32:$src, imm:$c;
    };
    expand {
        SHL $dst, $src, log2($unbound_c);
    };
};
)";

    EXPECT_FALSE(runPass(code));
}

/**
 * Verifies that the semantic pass rejects duplicate rewrite rule declarations with identical names.
 */
TEST_F(LegalizeRulePassTest, TestErrorDuplicateRuleName)
{
    std::string code = R"(
rule DuplicateRule {
    match {
        ADD i32:$dst, i32:$src, 0;
    };
    expand {
        COPY $dst, $src;
    };
};

rule DuplicateRule {
    match {
        SUB i32:$dst, i32:$src, 0;
    };
    expand {
        COPY $dst, $src;
    };
};
)";

    EXPECT_FALSE(runPass(code));
}