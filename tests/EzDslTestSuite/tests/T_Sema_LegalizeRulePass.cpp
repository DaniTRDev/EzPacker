#include "EzDslTestSuite.h"
#include "Ast/LegalizeRuleDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/LegalizeRuleDefLang.h"
#include "Parser/ParseContext.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/LegalizeRulePass.h"

class LegalizeRulePassTest : public DslTestSuiteAsGtest
{
  protected:
    std::unique_ptr<SymbolTable> m_table;

    void SetUp() override
    {
        DslTestSuiteAsGtest::SetUp();
        m_table = std::make_unique<SymbolTable>(getAllocator());

        registerPrimitiveTypes();
        registerDefaultIrInstructions();

        m_table->enterScope("TargetScope");
    }

    void registerType(std::string_view name, uint32_t bitWidth)
    {
        Sema::Symbols::TypeSymbol typeSym{ .m_name = name,
                                           .m_kind = static_cast<DSL::Ast::TypeDef::TypeKind>(0),
                                           .m_bitWidth = bitWidth };
        m_table->declareSym(nullptr, SymbolFlags::IsDefined, SymbolType::Type, typeSym, name);
    }

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

    std::optional<DSL::Ast::LegalizeRuleDef::TargetLegalizeRuleDef> parseFile(const std::string &source)
    {
        ParseContext ctx = createParseContextFromBuff("LegalizeRulePassTest", source);
        return ctx.parse<DSL::Parser::LegalizeRuleDef::TargetLegalizeRuleDef,
                         DSL::Ast::LegalizeRuleDef::TargetLegalizeRuleDef>();
    }

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