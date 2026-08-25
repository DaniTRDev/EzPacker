#include "EzDslTestSuite.h"
#include "Ast/LegalizeRuleDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Parser/LegalizeRuleDefLang.h"
#include "Parser/ParseContext.h"
#include "SourceManager/SourceManager.h"

/**
 * Test fixture for Legalize Rule Definition Language (.lrd) parser, rewrite rules, and AST construction.
 */
class LegalizeRuleDefLangTest : public DslTestSuiteAsGtest
{
  public:
};

// ============================================================================
// 1. Operand Parsing Tests
// ============================================================================

/**
 * Verifies parsing typed prefix SSA register operands (e.g. i32:$dst).
 */
TEST_F(LegalizeRuleDefLangTest, TestTypedPrefixSsaOperand)
{
    std::string test = "i32:$dst";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleOperand, DSL::Ast::LegalizeRuleDef::RuleOperand>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::SsaRegister);
    EXPECT_EQ(res->m_name.m_node, "dst");
    ASSERT_TRUE(res->m_type.has_value());
    EXPECT_EQ(res->m_type->m_node, "i32");
    EXPECT_FALSE(res->m_typeParam.has_value());
}

/**
 * Verifies parsing typed prefix immediate symbol operands (e.g. imm:$c).
 */
TEST_F(LegalizeRuleDefLangTest, TestTypedPrefixImmediateSymbolOperand)
{
    std::string test = "imm:$c";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleOperand, DSL::Ast::LegalizeRuleDef::RuleOperand>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateSymbol);
    EXPECT_EQ(res->m_name.m_node, "c");
    ASSERT_TRUE(res->m_type.has_value());
    EXPECT_EQ(res->m_type->m_node, "imm");
    EXPECT_FALSE(res->m_typeParam.has_value());
}

/**
 * Verifies parsing parameterized prefix immediate symbol operands (e.g. imm(i32):$c, simm(i12):$offset).
 */
TEST_F(LegalizeRuleDefLangTest, TestParameterizedPrefixImmediateSymbolOperand)
{
    std::string testTyped = "imm(i32):$c";
    ParseContext ctxTyped = createParseContextFromBuff("testTyped", testTyped);

    auto resTyped = ctxTyped.parse<DSL::Parser::LegalizeRuleDef::RuleOperand, DSL::Ast::LegalizeRuleDef::RuleOperand>();
    ASSERT_TRUE(resTyped.has_value());
    EXPECT_EQ(resTyped->m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateSymbol);
    EXPECT_EQ(resTyped->m_name.m_node, "c");
    ASSERT_TRUE(resTyped->m_type.has_value());
    EXPECT_EQ(resTyped->m_type->m_node, "imm");
    ASSERT_TRUE(resTyped->m_typeParam.has_value());
    EXPECT_EQ(resTyped->m_typeParam->m_node, "i32");

    std::string testWidth = "simm(i12):$offset";
    ParseContext ctxWidth = createParseContextFromBuff("testWidth", testWidth);

    auto resWidth = ctxWidth.parse<DSL::Parser::LegalizeRuleDef::RuleOperand, DSL::Ast::LegalizeRuleDef::RuleOperand>();
    ASSERT_TRUE(resWidth.has_value());
    EXPECT_EQ(resWidth->m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateSymbol);
    EXPECT_EQ(resWidth->m_name.m_node, "offset");
    ASSERT_TRUE(resWidth->m_type.has_value());
    EXPECT_EQ(resWidth->m_type->m_node, "simm");
    ASSERT_TRUE(resWidth->m_typeParam.has_value());
    EXPECT_EQ(resWidth->m_typeParam->m_node, "i12");
}

/**
 * Verifies parsing untyped bare SSA register references (e.g. $src).
 */
TEST_F(LegalizeRuleDefLangTest, TestBareSsaOperand)
{
    std::string test = "$src";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleOperand, DSL::Ast::LegalizeRuleDef::RuleOperand>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::SsaRegister);
    EXPECT_EQ(res->m_name.m_node, "src");
    EXPECT_FALSE(res->m_type.has_value());
    EXPECT_FALSE(res->m_typeParam.has_value());
}

/**
 * Verifies parsing immediate integer literals in decimal, negative decimal, and hexadecimal formats.
 */
TEST_F(LegalizeRuleDefLangTest, TestLiteralImmediateOperands)
{
    // Decimal literal
    std::string testDec = "42";
    ParseContext ctxDec = createParseContextFromBuff("testDec", testDec);

    auto resDec = ctxDec.parse<DSL::Parser::LegalizeRuleDef::RuleOperand, DSL::Ast::LegalizeRuleDef::RuleOperand>();
    ASSERT_TRUE(resDec.has_value());
    EXPECT_EQ(resDec->m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateLiteral);
    ASSERT_TRUE(resDec->m_immLiteral.has_value());
    EXPECT_EQ(resDec->m_immLiteral->m_node, 42);

    // Negative literal
    std::string testNeg = "-2048";
    ParseContext ctxNeg = createParseContextFromBuff("testNeg", testNeg);

    auto resNeg = ctxNeg.parse<DSL::Parser::LegalizeRuleDef::RuleOperand, DSL::Ast::LegalizeRuleDef::RuleOperand>();
    ASSERT_TRUE(resNeg.has_value());
    EXPECT_EQ(resNeg->m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateLiteral);
    ASSERT_TRUE(resNeg->m_immLiteral.has_value());
    EXPECT_EQ(resNeg->m_immLiteral->m_node, -2048);

    // Hexadecimal literal
    std::string testHex = "0xFF";
    ParseContext ctxHex = createParseContextFromBuff("testHex", testHex);

    auto resHex = ctxHex.parse<DSL::Parser::LegalizeRuleDef::RuleOperand, DSL::Ast::LegalizeRuleDef::RuleOperand>();
    ASSERT_TRUE(resHex.has_value());
    EXPECT_EQ(resHex->m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateLiteral);
    ASSERT_TRUE(resHex->m_immLiteral.has_value());
    EXPECT_EQ(resHex->m_immLiteral->m_node, 0xFF);
}

/**
 * Verifies parsing single-argument custom transformation functions (e.g. log2($shift)).
 */
TEST_F(LegalizeRuleDefLangTest, TestCustomTransformOperand)
{
    std::string test = "log2($shift)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleOperand, DSL::Ast::LegalizeRuleDef::RuleOperand>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::CustomTransform);
    EXPECT_EQ(res->m_name.m_node, "log2");
    ASSERT_EQ(res->m_callArgs.size(), 1);
    EXPECT_EQ(res->m_callArgs[0].m_node, "shift");
}

/**
 * Verifies parsing multi-argument custom transformation functions (e.g. combineBits($hi, $lo)).
 */
TEST_F(LegalizeRuleDefLangTest, TestCustomTransformMultiArgOperand)
{
    std::string test = "combineBits($hi, $lo)";
    ParseContext ctx = createParseContextFromBuff("test", test);

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

/**
 * Verifies parsing zero-operand rule instruction patterns (e.g. NOP;).
 */
TEST_F(LegalizeRuleDefLangTest, TestInstructionWithoutOperands)
{
    std::string test = "NOP;";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleInstruction, DSL::Ast::LegalizeRuleDef::RuleInstruction>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_opcode.m_node, "NOP");
    EXPECT_TRUE(res->m_operands.empty());
}

/**
 * Verifies parsing rule instructions containing heterogeneous combinations of typed registers,
 * bare registers, immediate symbols, literal constants, and custom transforms.
 */
TEST_F(LegalizeRuleDefLangTest, TestInstructionWithMixedOperands)
{
    std::string test = "ADD i32:$dst, $lhs, imm(i32):$c, 42, log2($shift);";
    ParseContext ctx = createParseContextFromBuff("test", test);

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

/**
 * Verifies parsing single-argument semantic predicate guards in when clauses (e.g. isPowTwo($c);).
 */
TEST_F(LegalizeRuleDefLangTest, TestRulePredicateSingleArg)
{
    std::string test = "isPowTwo($c);";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RulePredicate, DSL::Ast::LegalizeRuleDef::RulePredicate>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_predicateName.m_node, "isPowTwo");
    ASSERT_EQ(res->m_arguments.size(), 1);

    ASSERT_TRUE(std::holds_alternative<DSL::Ast::Common::Identifier>(res->m_arguments[0]));
    EXPECT_EQ(std::get<DSL::Ast::Common::Identifier>(res->m_arguments[0]).m_node, "c");
}

/**
 * Verifies parsing multi-argument semantic predicate guards in when clauses (e.g. isAddCarryLegal($lhs, $rhs);).
 */
TEST_F(LegalizeRuleDefLangTest, TestRulePredicateMultiArg)
{
    std::string test = "isAddCarryLegal($lhs, $rhs);";
    ParseContext ctx = createParseContextFromBuff("test", test);

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

/**
 * Verifies parsing a complete rewrite rule in standard section ordering (match -> when -> expand).
 */
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
    ParseContext ctx = createParseContextFromBuff("test", test);

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

/**
 * Verifies parsing rewrite rules with permuted block ordering (expand -> when -> match).
 */
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
    ParseContext ctx = createParseContextFromBuff("test", test);

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

/**
 * Verifies parsing rewrite rules that do not declare a when predicate guard block.
 */
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
    ParseContext ctx = createParseContextFromBuff("test", test);

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

/**
 * Verifies parsing an entire target legalization rule file (.lrd) containing multiple rewrite rules.
 */
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

    ParseContext ctx = createParseContextFromBuff("test", test);

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

/**
 * Verifies syntax error rejection when disallowed postfix immediate notation ($c:imm) is used.
 */
TEST_F(LegalizeRuleDefLangTest, TestDisallowedPostfixImmediateSyntaxError)
{
    std::string test = "ADD i32:$dst, i32:$lhs, $c:imm;";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleInstruction, DSL::Ast::LegalizeRuleDef::RuleInstruction>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when disallowed postfix type notation ($dst:i32) is used.
 */
TEST_F(LegalizeRuleDefLangTest, TestDisallowedPostfixTypeSyntaxError)
{
    std::string test = "ADD $dst:i32, $src:i32, 42;";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleInstruction, DSL::Ast::LegalizeRuleDef::RuleInstruction>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when a semicolon is missing after an inner block in a rule.
 */
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
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::TargetLegalizeRuleDef,
                         DSL::Ast::LegalizeRuleDef::TargetLegalizeRuleDef>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when a semicolon is missing after a top-level rule declaration in a file.
 */
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
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::TargetLegalizeRuleDef,
                         DSL::Ast::LegalizeRuleDef::TargetLegalizeRuleDef>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when a semicolon is missing at the end of an instruction.
 */
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
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::LegalizeRewriteRule,
                         DSL::Ast::LegalizeRuleDef::LegalizeRewriteRule>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when a semicolon is missing inside a predicate guard.
 */
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
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::LegalizeRewriteRule,
                         DSL::Ast::LegalizeRuleDef::LegalizeRewriteRule>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection on empty dollar sign variable syntax ($;).
 */
TEST_F(LegalizeRuleDefLangTest, TestInvalidDollarVariableSyntaxError)
{
    std::string test = "ADD $; ";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleInstruction, DSL::Ast::LegalizeRuleDef::RuleInstruction>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when an expand block has an unclosed brace.
 */
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
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::LegalizeRewriteRule,
                         DSL::Ast::LegalizeRuleDef::LegalizeRewriteRule>();
    EXPECT_FALSE(res.has_value());
}