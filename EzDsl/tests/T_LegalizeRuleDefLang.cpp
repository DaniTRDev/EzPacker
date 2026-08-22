#include "EzDslCommon.h"
#include "Ast/LegalizeRuleDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Parser/LegalizeRuleDefLang.h"
#include "Parser/ParseContext.h"
#include "SourceManager/SourceManager.h"
#include <gtest/gtest.h>

class LegalizeRuleDefLangTest : public ::testing::Test
{
  public:
    DiagnosticCollector *getDiagCollector() { return m_diagCollector.get(); }

    size_t addSource(const std::string &source, const std::string &content)
    {
        return m_sourceManager->addSourceContent(source, content);
    }

    SourceManager *getSourceManager() { return m_sourceManager.get(); }

    void SetUp() override
    {
        m_sourceManager = std::make_shared<SourceManager>("", &m_resource);
        m_diagLogger = std::make_shared<DiagnosticLogger>(m_sourceManager.get());
        m_diagCollector = std::make_shared<DiagnosticCollector>();

        m_diagCollector->addListener(m_diagLogger.get());
    }

    void TearDown() override {}

  private:
    std::pmr::monotonic_buffer_resource m_resource;
    std::shared_ptr<DiagnosticCollector> m_diagCollector;
    std::shared_ptr<DiagnosticLogger> m_diagLogger;
    std::shared_ptr<SourceManager> m_sourceManager;
};

// ============================================================================
// 1. Operand Parsing Tests
// ============================================================================

TEST_F(LegalizeRuleDefLangTest, TestTypedPrefixSsaOperand)
{
    std::string test = "i32:$dst";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleOperand, DSL::Ast::LegalizeRuleDef::RuleOperand>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::SsaRegister);
    EXPECT_EQ(res->m_name.m_node, "dst");
    ASSERT_TRUE(res->m_type.has_value());
    EXPECT_EQ(res->m_type->m_node, "i32");
    EXPECT_FALSE(res->m_typeParam.has_value());
}

TEST_F(LegalizeRuleDefLangTest, TestTypedPrefixImmediateSymbolOperand)
{
    std::string test = "imm:$c";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleOperand, DSL::Ast::LegalizeRuleDef::RuleOperand>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateSymbol);
    EXPECT_EQ(res->m_name.m_node, "c");
    ASSERT_TRUE(res->m_type.has_value());
    EXPECT_EQ(res->m_type->m_node, "imm");
    EXPECT_FALSE(res->m_typeParam.has_value());
}

TEST_F(LegalizeRuleDefLangTest, TestParameterizedPrefixImmediateSymbolOperand)
{
    std::string testTyped = "imm(i32):$c";
    size_t sourceIdTyped = addSource("testTyped", testTyped);
    ParseContext ctxTyped(getDiagCollector(), getSourceManager(), sourceIdTyped);

    auto resTyped = ctxTyped.parse<DSL::Parser::LegalizeRuleDef::RuleOperand, DSL::Ast::LegalizeRuleDef::RuleOperand>();
    ASSERT_TRUE(resTyped.has_value());
    EXPECT_EQ(resTyped->m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateSymbol);
    EXPECT_EQ(resTyped->m_name.m_node, "c");
    ASSERT_TRUE(resTyped->m_type.has_value());
    EXPECT_EQ(resTyped->m_type->m_node, "imm");
    ASSERT_TRUE(resTyped->m_typeParam.has_value());
    EXPECT_EQ(resTyped->m_typeParam->m_node, "i32");

    std::string testWidth = "simm(i12):$offset";
    size_t sourceIdWidth = addSource("testWidth", testWidth);
    ParseContext ctxWidth(getDiagCollector(), getSourceManager(), sourceIdWidth);

    auto resWidth = ctxWidth.parse<DSL::Parser::LegalizeRuleDef::RuleOperand, DSL::Ast::LegalizeRuleDef::RuleOperand>();
    ASSERT_TRUE(resWidth.has_value());
    EXPECT_EQ(resWidth->m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateSymbol);
    EXPECT_EQ(resWidth->m_name.m_node, "offset");
    ASSERT_TRUE(resWidth->m_type.has_value());
    EXPECT_EQ(resWidth->m_type->m_node, "simm");
    ASSERT_TRUE(resWidth->m_typeParam.has_value());
    EXPECT_EQ(resWidth->m_typeParam->m_node, "i12");
}

TEST_F(LegalizeRuleDefLangTest, TestBareSsaOperand)
{
    std::string test = "$src";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleOperand, DSL::Ast::LegalizeRuleDef::RuleOperand>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::SsaRegister);
    EXPECT_EQ(res->m_name.m_node, "src");
    EXPECT_FALSE(res->m_type.has_value());
    EXPECT_FALSE(res->m_typeParam.has_value());
}

TEST_F(LegalizeRuleDefLangTest, TestLiteralImmediateOperands)
{
    // Decimal literal
    std::string testDec = "42";
    size_t sourceIdDec = addSource("testDec", testDec);
    ParseContext ctxDec(getDiagCollector(), getSourceManager(), sourceIdDec);

    auto resDec = ctxDec.parse<DSL::Parser::LegalizeRuleDef::RuleOperand, DSL::Ast::LegalizeRuleDef::RuleOperand>();
    ASSERT_TRUE(resDec.has_value());
    EXPECT_EQ(resDec->m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateLiteral);
    ASSERT_TRUE(resDec->m_immLiteral.has_value());
    EXPECT_EQ(resDec->m_immLiteral->m_node, 42);

    // Negative literal
    std::string testNeg = "-2048";
    size_t sourceIdNeg = addSource("testNeg", testNeg);
    ParseContext ctxNeg(getDiagCollector(), getSourceManager(), sourceIdNeg);

    auto resNeg = ctxNeg.parse<DSL::Parser::LegalizeRuleDef::RuleOperand, DSL::Ast::LegalizeRuleDef::RuleOperand>();
    ASSERT_TRUE(resNeg.has_value());
    EXPECT_EQ(resNeg->m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateLiteral);
    ASSERT_TRUE(resNeg->m_immLiteral.has_value());
    EXPECT_EQ(resNeg->m_immLiteral->m_node, -2048);

    // Hexadecimal literal
    std::string testHex = "0xFF";
    size_t sourceIdHex = addSource("testHex", testHex);
    ParseContext ctxHex(getDiagCollector(), getSourceManager(), sourceIdHex);

    auto resHex = ctxHex.parse<DSL::Parser::LegalizeRuleDef::RuleOperand, DSL::Ast::LegalizeRuleDef::RuleOperand>();
    ASSERT_TRUE(resHex.has_value());
    EXPECT_EQ(resHex->m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateLiteral);
    ASSERT_TRUE(resHex->m_immLiteral.has_value());
    EXPECT_EQ(resHex->m_immLiteral->m_node, 0xFF);
}

TEST_F(LegalizeRuleDefLangTest, TestCustomTransformOperand)
{
    std::string test = "log2($shift)";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleOperand, DSL::Ast::LegalizeRuleDef::RuleOperand>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::CustomTransform);
    EXPECT_EQ(res->m_name.m_node, "log2");
    ASSERT_EQ(res->m_callArgs.size(), 1);
    EXPECT_EQ(res->m_callArgs[0].m_node, "shift");
}

TEST_F(LegalizeRuleDefLangTest, TestCustomTransformMultiArgOperand)
{
    std::string test = "combineBits($hi, $lo)";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleOperand, DSL::Ast::LegalizeRuleDef::RuleOperand>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::CustomTransform);
    EXPECT_EQ(res->m_name.m_node, "combineBits");
    ASSERT_EQ(res->m_callArgs.size(), 2);
    EXPECT_EQ(res->m_callArgs[0].m_node, "hi");
    EXPECT_EQ(res->m_callArgs[1].m_node, "lo");
}

// ============================================================================
// 2. RuleInstruction Parsing Tests
// ============================================================================

TEST_F(LegalizeRuleDefLangTest, TestInstructionWithoutOperands)
{
    std::string test = "NOP;";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleInstruction, DSL::Ast::LegalizeRuleDef::RuleInstruction>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_opcode.m_node, "NOP");
    EXPECT_TRUE(res->m_operands.empty());
}

TEST_F(LegalizeRuleDefLangTest, TestInstructionWithMixedOperands)
{
    std::string test = "ADD i32:$dst, $lhs, imm(i32):$c, 42, log2($shift);";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleInstruction, DSL::Ast::LegalizeRuleDef::RuleInstruction>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_opcode.m_node, "ADD");
    ASSERT_EQ(res->m_operands.size(), 5);

    // 0: i32:$dst
    EXPECT_EQ(res->m_operands[0].m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::SsaRegister);
    EXPECT_EQ(res->m_operands[0].m_name.m_node, "dst");
    EXPECT_EQ(res->m_operands[0].m_type->m_node, "i32");

    // 1: $lhs
    EXPECT_EQ(res->m_operands[1].m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::SsaRegister);
    EXPECT_EQ(res->m_operands[1].m_name.m_node, "lhs");
    EXPECT_FALSE(res->m_operands[1].m_type.has_value());

    // 2: imm(i32):$c
    EXPECT_EQ(res->m_operands[2].m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateSymbol);
    EXPECT_EQ(res->m_operands[2].m_name.m_node, "c");
    EXPECT_EQ(res->m_operands[2].m_type->m_node, "imm");
    ASSERT_TRUE(res->m_operands[2].m_typeParam.has_value());
    EXPECT_EQ(res->m_operands[2].m_typeParam->m_node, "i32");

    // 3: 42
    EXPECT_EQ(res->m_operands[3].m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateLiteral);
    EXPECT_EQ(res->m_operands[3].m_immLiteral->m_node, 42);

    // 4: log2($shift)
    EXPECT_EQ(res->m_operands[4].m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::CustomTransform);
    EXPECT_EQ(res->m_operands[4].m_name.m_node, "log2");
    ASSERT_EQ(res->m_operands[4].m_callArgs.size(), 1);
    EXPECT_EQ(res->m_operands[4].m_callArgs[0].m_node, "shift");
}

// ============================================================================
// 3. RulePredicate (When Guard) Parsing Tests
// ============================================================================

TEST_F(LegalizeRuleDefLangTest, TestRulePredicateSingleArg)
{
    std::string test = "isPowTwo($c);";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RulePredicate, DSL::Ast::LegalizeRuleDef::RulePredicate>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_predicateName.m_node, "isPowTwo");
    ASSERT_EQ(res->m_arguments.size(), 1);

    ASSERT_TRUE(std::holds_alternative<DSL::Ast::Common::Identifier>(res->m_arguments[0]));
    EXPECT_EQ(std::get<DSL::Ast::Common::Identifier>(res->m_arguments[0]).m_node, "c");
}

TEST_F(LegalizeRuleDefLangTest, TestRulePredicateMultiArg)
{
    std::string test = "isAddCarryLegal($lhs, $rhs);";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RulePredicate, DSL::Ast::LegalizeRuleDef::RulePredicate>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_predicateName.m_node, "isAddCarryLegal");
    ASSERT_EQ(res->m_arguments.size(), 2);

    ASSERT_TRUE(std::holds_alternative<DSL::Ast::Common::Identifier>(res->m_arguments[0]));
    EXPECT_EQ(std::get<DSL::Ast::Common::Identifier>(res->m_arguments[0]).m_node, "lhs");
    ASSERT_TRUE(std::holds_alternative<DSL::Ast::Common::Identifier>(res->m_arguments[1]));
    EXPECT_EQ(std::get<DSL::Ast::Common::Identifier>(res->m_arguments[1]).m_node, "rhs");
}

// ============================================================================
// 4. LegalizeRewriteRule Parsing Tests (Block Permutations)
// ============================================================================

TEST_F(LegalizeRuleDefLangTest, TestCompleteRewriteRuleStandardOrder)
{
    std::string test = R"dsl(
rule SDivPow2 {
    match {
        SDIV i32:$dst, i32:$lhs, imm(i32):$c;
    };
    when {
        isPowTwo($c);
        isPositiveConst($c);
    };
    expand {
        SAR i32:$dst, i32:$lhs, log2($c);
    };
}
)dsl";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::LegalizeRewriteRule,
                         DSL::Ast::LegalizeRuleDef::LegalizeRewriteRule>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_ruleName.m_node, "SDivPow2");

    // Match patterns
    ASSERT_EQ(res->m_matchPatterns.size(), 1);
    EXPECT_EQ(res->m_matchPatterns[0].m_opcode.m_node, "SDIV");
    ASSERT_EQ(res->m_matchPatterns[0].m_operands.size(), 3);
    EXPECT_EQ(res->m_matchPatterns[0].m_operands[0].m_name.m_node, "dst");
    EXPECT_EQ(res->m_matchPatterns[0].m_operands[2].m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateSymbol);
    EXPECT_EQ(res->m_matchPatterns[0].m_operands[2].m_name.m_node, "c");
    ASSERT_TRUE(res->m_matchPatterns[0].m_operands[2].m_typeParam.has_value());
    EXPECT_EQ(res->m_matchPatterns[0].m_operands[2].m_typeParam->m_node, "i32");

    // Predicates
    ASSERT_EQ(res->m_predicates.size(), 2);
    EXPECT_EQ(res->m_predicates[0].m_predicateName.m_node, "isPowTwo");
    ASSERT_TRUE(std::holds_alternative<DSL::Ast::Common::Identifier>(res->m_predicates[0].m_arguments[0]));
    EXPECT_EQ(std::get<DSL::Ast::Common::Identifier>(res->m_predicates[0].m_arguments[0]).m_node, "c");

    EXPECT_EQ(res->m_predicates[1].m_predicateName.m_node, "isPositiveConst");
    ASSERT_TRUE(std::holds_alternative<DSL::Ast::Common::Identifier>(res->m_predicates[1].m_arguments[0]));
    EXPECT_EQ(std::get<DSL::Ast::Common::Identifier>(res->m_predicates[1].m_arguments[0]).m_node, "c");

    // Expansion sequence
    ASSERT_EQ(res->m_expansionSequence.size(), 1);
    EXPECT_EQ(res->m_expansionSequence[0].m_opcode.m_node, "SAR");
    ASSERT_EQ(res->m_expansionSequence[0].m_operands.size(), 3);
    EXPECT_EQ(res->m_expansionSequence[0].m_operands[2].m_kind,
              DSL::Ast::LegalizeRuleDef::OperandKind::CustomTransform);
    EXPECT_EQ(res->m_expansionSequence[0].m_operands[2].m_name.m_node, "log2");
}

TEST_F(LegalizeRuleDefLangTest, TestRewriteRulePermutedBlocksOrder)
{
    // Order: expand -> when -> match
    std::string test = R"dsl(
rule PermutedOrder {
    expand {
        SAR i32:$dst, i32:$lhs, log2($c);
    };
    when {
        isPowTwo($c);
    };
    match {
        SDIV i32:$dst, i32:$lhs, imm:$c;
    };
}
)dsl";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::LegalizeRewriteRule,
                         DSL::Ast::LegalizeRuleDef::LegalizeRewriteRule>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_ruleName.m_node, "PermutedOrder");

    ASSERT_EQ(res->m_matchPatterns.size(), 1);
    EXPECT_EQ(res->m_matchPatterns[0].m_opcode.m_node, "SDIV");

    ASSERT_EQ(res->m_predicates.size(), 1);
    EXPECT_EQ(res->m_predicates[0].m_predicateName.m_node, "isPowTwo");

    ASSERT_EQ(res->m_expansionSequence.size(), 1);
    EXPECT_EQ(res->m_expansionSequence[0].m_opcode.m_node, "SAR");
}

TEST_F(LegalizeRuleDefLangTest, TestRewriteRuleWithoutWhenBlock)
{
    std::string test = R"dsl(
rule NarrowAddi64 {
    match {
        ADD i64:$dst, i64:$lhs, i64:$rhs;
    };
    expand {
        UNMERGE_VALUES i32:$lhs_lo, i32:$lhs_hi, i64:$lhs;
        UNMERGE_VALUES i32:$rhs_lo, i32:$rhs_hi, i64:$rhs;
        UADDO i32:$dst_lo, i1:$carry, i32:$lhs_lo, i32:$rhs_lo;
        UADDE i32:$dst_hi, i1:$carry_out, i32:$lhs_hi, i32:$rhs_hi, i1:$carry;
        MERGE_VALUES i64:$dst, i32:$dst_lo, i32:$dst_hi;
    };
}
)dsl";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::LegalizeRewriteRule,
                         DSL::Ast::LegalizeRuleDef::LegalizeRewriteRule>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_ruleName.m_node, "NarrowAddi64");

    ASSERT_EQ(res->m_matchPatterns.size(), 1);
    EXPECT_EQ(res->m_matchPatterns[0].m_opcode.m_node, "ADD");
    EXPECT_TRUE(res->m_predicates.empty());

    ASSERT_EQ(res->m_expansionSequence.size(), 5);
    EXPECT_EQ(res->m_expansionSequence[0].m_opcode.m_node, "UNMERGE_VALUES");
    EXPECT_EQ(res->m_expansionSequence[2].m_opcode.m_node, "UADDO");
    EXPECT_EQ(res->m_expansionSequence[3].m_opcode.m_node, "UADDE");
    EXPECT_EQ(res->m_expansionSequence[4].m_opcode.m_node, "MERGE_VALUES");
}

// ============================================================================
// 5. Full Target Legalize Rule Translation Unit Tests
// ============================================================================

TEST_F(LegalizeRuleDefLangTest, TestFullTargetLegalizeRuleDef)
{
    std::string test = R"dsl(
rule SDivPow2 {
    match {
        SDIV i32:$dst, i32:$lhs, imm(i32):$c;
    };
    when {
        isPowTwo($c);
    };
    expand {
        SAR i32:$dst, i32:$lhs, log2($c);
    };
};

rule NarrowAddi64 {
    match {
        ADD i64:$dst, i64:$lhs, i64:$rhs;
    };
    expand {
        UNMERGE_VALUES i32:$lhs_lo, i32:$lhs_hi, i64:$lhs;
        UNMERGE_VALUES i32:$rhs_lo, i32:$rhs_hi, i64:$rhs;
        UADDO i32:$dst_lo, i1:$carry, i32:$lhs_lo, i32:$rhs_lo;
        UADDE i32:$dst_hi, i1:$carry_out, i32:$lhs_hi, i32:$rhs_hi, i1:$carry;
        MERGE_VALUES i64:$dst, i32:$dst_lo, i32:$dst_hi;
    };
};
)dsl";

    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::TargetLegalizeRuleDef,
                         DSL::Ast::LegalizeRuleDef::TargetLegalizeRuleDef>();
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->m_rules.size(), 2);

    EXPECT_EQ(res->m_rules[0].m_ruleName.m_node, "SDivPow2");
    EXPECT_EQ(res->m_rules[1].m_ruleName.m_node, "NarrowAddi64");
}

// ============================================================================
// 6. Negative & Syntax Error Tests
// ============================================================================

TEST_F(LegalizeRuleDefLangTest, TestDisallowedPostfixImmediateSyntaxError)
{
    std::string test = "ADD i32:$dst, i32:$lhs, $c:imm;";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleInstruction, DSL::Ast::LegalizeRuleDef::RuleInstruction>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(LegalizeRuleDefLangTest, TestDisallowedPostfixTypeSyntaxError)
{
    std::string test = "ADD $dst:i32, $src:i32, 42;";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleInstruction, DSL::Ast::LegalizeRuleDef::RuleInstruction>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(LegalizeRuleDefLangTest, TestMissingSemicolonAfterBlockError)
{
    std::string test = R"dsl(
rule BadRule {
    match {
        ADD i32:$dst, i32:$lhs, i32:$rhs;
    }
    expand {
        NOP;
    };
};
)dsl";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::TargetLegalizeRuleDef,
                         DSL::Ast::LegalizeRuleDef::TargetLegalizeRuleDef>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(LegalizeRuleDefLangTest, TestMissingSemicolonAfterRuleInFileError)
{
    std::string test = R"dsl(
rule BadRule {
    match {
        ADD i32:$dst, i32:$lhs, i32:$rhs;
    };
    expand {
        NOP;
    };
}
)dsl";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::TargetLegalizeRuleDef,
                         DSL::Ast::LegalizeRuleDef::TargetLegalizeRuleDef>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(LegalizeRuleDefLangTest, TestMissingSemicolonInInstructionError)
{
    std::string test = R"dsl(
rule BadRule {
    match {
        ADD i32:$dst, i32:$lhs, i32:$rhs
    };
    expand {
        NOP;
    };
}
)dsl";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::LegalizeRewriteRule,
                         DSL::Ast::LegalizeRuleDef::LegalizeRewriteRule>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(LegalizeRuleDefLangTest, TestMissingSemicolonInPredicateError)
{
    std::string test = R"dsl(
rule BadRule {
    match {
        ADD i32:$dst, i32:$lhs, imm:$c;
    };
    when {
        isPowTwo($c)
    };
    expand {
        NOP;
    };
}
)dsl";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::LegalizeRewriteRule,
                         DSL::Ast::LegalizeRuleDef::LegalizeRewriteRule>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(LegalizeRuleDefLangTest, TestInvalidDollarVariableSyntaxError)
{
    std::string test = "ADD $; ";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleInstruction, DSL::Ast::LegalizeRuleDef::RuleInstruction>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(LegalizeRuleDefLangTest, TestUnclosedBraceInExpandBlockError)
{
    std::string test = R"dsl(
rule BadRule {
    match {
        NOP;
    };
    expand {
        NOP;
)dsl";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::LegalizeRewriteRule,
                         DSL::Ast::LegalizeRuleDef::LegalizeRewriteRule>();
    EXPECT_FALSE(res.has_value());
}