#include "EzDslLexerTestSuite.h"
#include "Ast/CommonAstNodes.h"
#include "Ast/LegalizeRuleDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Parser/LegalizeRuleDefLang.h"
#include "Parser/ParseContext.h"
#include "SourceManager/SourceManager.h"

class LegalizeRuleDefLangTest : public DslLexerTestSuiteAsGtest
{
};

// ============================================================================
// 1. Operand Parsing Tests
// ============================================================================

TEST_F(LegalizeRuleDefLangTest, TestTypedPrefixSsaOperand)
{
    std::string test = "i32:$dst";
    ParseContext ctx = createParseContextFromBuff("test_operand_typed_prefix_ssa", test);

    auto res =
            ctx.parse<DSL::Parser::LegalizeRuleDef::RuleOperand, DSL::Ast::LegalizeRuleDef::RuleInstructionOperand>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::LegalizeRuleDef::RuleOperandKind::SsaRegister);
    EXPECT_EQ(res->m_name.m_node, "dst");
    ASSERT_TRUE(res->m_type.has_value());
    EXPECT_EQ(res->m_type->m_node, "i32");
    EXPECT_FALSE(res->m_typeParam.has_value());
}

TEST_F(LegalizeRuleDefLangTest, TestImmediateSymbolOperand)
{
    std::string test = "imm:$c";
    ParseContext ctx = createParseContextFromBuff("test_operand_imm_symbol", test);

    auto res =
            ctx.parse<DSL::Parser::LegalizeRuleDef::RuleOperand, DSL::Ast::LegalizeRuleDef::RuleInstructionOperand>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::LegalizeRuleDef::RuleOperandKind::ImmediateSymbol);
    EXPECT_EQ(res->m_name.m_node, "c");
    ASSERT_TRUE(res->m_type.has_value());
    EXPECT_EQ(res->m_type->m_node, "imm");
    EXPECT_FALSE(res->m_typeParam.has_value());
}

TEST_F(LegalizeRuleDefLangTest, TestParameterizedImmediateSymbolOperand)
{
    std::string testTyped = "imm(i32):$c";
    ParseContext ctxTyped = createParseContextFromBuff("test_operand_param_imm_typed", testTyped);

    auto resTyped = ctxTyped.parse<DSL::Parser::LegalizeRuleDef::RuleOperand,
                                   DSL::Ast::LegalizeRuleDef::RuleInstructionOperand>();
    ASSERT_TRUE(resTyped.has_value());
    EXPECT_EQ(resTyped->m_kind, DSL::Ast::LegalizeRuleDef::RuleOperandKind::ImmediateSymbol);
    EXPECT_EQ(resTyped->m_name.m_node, "c");
    ASSERT_TRUE(resTyped->m_type.has_value());
    EXPECT_EQ(resTyped->m_type->m_node, "imm");
    ASSERT_TRUE(resTyped->m_typeParam.has_value());
    EXPECT_EQ(resTyped->m_typeParam->m_node, "i32");

    std::string testWidth = "imm(i12):$offset";
    ParseContext ctxWidth = createParseContextFromBuff("test_operand_param_imm_width", testWidth);

    auto resWidth = ctxWidth.parse<DSL::Parser::LegalizeRuleDef::RuleOperand,
                                   DSL::Ast::LegalizeRuleDef::RuleInstructionOperand>();
    ASSERT_TRUE(resWidth.has_value());
    EXPECT_EQ(resWidth->m_kind, DSL::Ast::LegalizeRuleDef::RuleOperandKind::ImmediateSymbol);
    EXPECT_EQ(resWidth->m_name.m_node, "offset");
    ASSERT_TRUE(resWidth->m_type.has_value());
    EXPECT_EQ(resWidth->m_type->m_node, "imm");
    ASSERT_TRUE(resWidth->m_typeParam.has_value());
    EXPECT_EQ(resWidth->m_typeParam->m_node, "i12");
}

TEST_F(LegalizeRuleDefLangTest, TestBareSsaOperand)
{
    std::string test = "$src";
    ParseContext ctx = createParseContextFromBuff("test_operand_bare_ssa", test);

    auto res =
            ctx.parse<DSL::Parser::LegalizeRuleDef::RuleOperand, DSL::Ast::LegalizeRuleDef::RuleInstructionOperand>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::LegalizeRuleDef::RuleOperandKind::SsaRegister);
    EXPECT_EQ(res->m_name.m_node, "src");
    EXPECT_FALSE(res->m_type.has_value());
    EXPECT_FALSE(res->m_typeParam.has_value());
}

TEST_F(LegalizeRuleDefLangTest, TestLiteralImmediateOperands)
{
    // Decimal literal
    {
        std::string test = "42";
        ParseContext ctx = createParseContextFromBuff("test_operand_lit_dec", test);
        auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleOperand,
                             DSL::Ast::LegalizeRuleDef::RuleInstructionOperand>();
        ASSERT_TRUE(res.has_value());
        EXPECT_EQ(res->m_kind, DSL::Ast::LegalizeRuleDef::RuleOperandKind::ImmediateLiteral);
        ASSERT_TRUE(res->m_immLiteral.has_value());
        EXPECT_EQ(res->m_immLiteral->m_node, 42);
    }

    // Negative literal
    {
        std::string test = "-2048";
        ParseContext ctx = createParseContextFromBuff("test_operand_lit_neg", test);
        auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleOperand,
                             DSL::Ast::LegalizeRuleDef::RuleInstructionOperand>();
        ASSERT_TRUE(res.has_value());
        EXPECT_EQ(res->m_kind, DSL::Ast::LegalizeRuleDef::RuleOperandKind::ImmediateLiteral);
        ASSERT_TRUE(res->m_immLiteral.has_value());
        EXPECT_EQ(res->m_immLiteral->m_node, -2048);
    }

    // Hexadecimal literal
    {
        std::string test = "0xFF";
        ParseContext ctx = createParseContextFromBuff("test_operand_lit_hex", test);
        auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleOperand,
                             DSL::Ast::LegalizeRuleDef::RuleInstructionOperand>();
        ASSERT_TRUE(res.has_value());
        EXPECT_EQ(res->m_kind, DSL::Ast::LegalizeRuleDef::RuleOperandKind::ImmediateLiteral);
        ASSERT_TRUE(res->m_immLiteral.has_value());
        EXPECT_EQ(res->m_immLiteral->m_node, 0xFF);
    }
}

TEST_F(LegalizeRuleDefLangTest, TestCustomTransformOperands)
{
    // Single-argument transform
    {
        std::string test = "log2($shift)";
        ParseContext ctx = createParseContextFromBuff("test_operand_cust_single", test);
        auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleOperand,
                             DSL::Ast::LegalizeRuleDef::RuleInstructionOperand>();
        ASSERT_TRUE(res.has_value());
        EXPECT_EQ(res->m_kind, DSL::Ast::LegalizeRuleDef::RuleOperandKind::CustomTransform);
        EXPECT_EQ(res->m_name.m_node, "log2");
        ASSERT_EQ(res->m_callArgs.size(), 1);
        EXPECT_EQ(res->m_callArgs[0].m_node, "shift");
    }

    // Multi-argument transform
    {
        std::string test = "combineBits($hi, $lo)";
        ParseContext ctx = createParseContextFromBuff("test_operand_cust_multi", test);
        auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleOperand,
                             DSL::Ast::LegalizeRuleDef::RuleInstructionOperand>();
        ASSERT_TRUE(res.has_value());
        EXPECT_EQ(res->m_kind, DSL::Ast::LegalizeRuleDef::RuleOperandKind::CustomTransform);
        EXPECT_EQ(res->m_name.m_node, "combineBits");
        ASSERT_EQ(res->m_callArgs.size(), 2);
        EXPECT_EQ(res->m_callArgs[0].m_node, "hi");
        EXPECT_EQ(res->m_callArgs[1].m_node, "lo");
    }
}

TEST_F(LegalizeRuleDefLangTest, TestDisallowedOperandFormats)
{
    // Postfix type notation ($c:imm) in an instruction statement
    {
        std::string test = "ADD i32:$dst, i32:$lhs, $c:imm;";
        ParseContext ctx = createParseContextFromBuff("test_disallow_postfix_imm", test);
        auto res =
                ctx.parse<DSL::Parser::LegalizeRuleDef::RuleInstruction, DSL::Ast::LegalizeRuleDef::RuleInstruction>();
        EXPECT_FALSE(res.has_value());
    }

    // Postfix type notation ($dst:i32) in an instruction statement
    {
        std::string test = "ADD $dst:i32, $src:i32, 42;";
        ParseContext ctx = createParseContextFromBuff("test_disallow_postfix_type", test);
        auto res =
                ctx.parse<DSL::Parser::LegalizeRuleDef::RuleInstruction, DSL::Ast::LegalizeRuleDef::RuleInstruction>();
        EXPECT_FALSE(res.has_value());
    }

    // Lone dollar sign without identifier
    {
        std::string test = "ADD $;";
        ParseContext ctx = createParseContextFromBuff("test_disallow_lone_dollar", test);
        auto res =
                ctx.parse<DSL::Parser::LegalizeRuleDef::RuleInstruction, DSL::Ast::LegalizeRuleDef::RuleInstruction>();
        EXPECT_FALSE(res.has_value());
    }
}

// ============================================================================
// 2. RuleInstruction Parsing Tests
// ============================================================================

TEST_F(LegalizeRuleDefLangTest, TestInstructionVariations)
{
    {
        std::string test = "NOP;";
        ParseContext ctx = createParseContextFromBuff("test_inst_nop", test);
        auto res =
                ctx.parse<DSL::Parser::LegalizeRuleDef::RuleInstruction, DSL::Ast::LegalizeRuleDef::RuleInstruction>();
        ASSERT_TRUE(res.has_value());
        EXPECT_EQ(res->m_opcode.m_node, "NOP");
        EXPECT_TRUE(res->m_operands.empty());
    }

    {
        std::string test = "$dst;";
        ParseContext ctx = createParseContextFromBuff("test_inst_bare_dst", test);
        auto res =
                ctx.parse<DSL::Parser::LegalizeRuleDef::RuleInstruction, DSL::Ast::LegalizeRuleDef::RuleInstruction>();
        ASSERT_TRUE(res.has_value());
        EXPECT_EQ(res->m_opcode.m_node, "dst");
        ASSERT_EQ(res->m_operands.size(), 1);
        EXPECT_EQ(res->m_operands[0].m_kind, DSL::Ast::LegalizeRuleDef::RuleOperandKind::SsaRegister);
    }

    {
        std::string test = "i32:$dst;";
        ParseContext ctx = createParseContextFromBuff("test_inst_typed_dst", test);
        auto res =
                ctx.parse<DSL::Parser::LegalizeRuleDef::RuleInstruction, DSL::Ast::LegalizeRuleDef::RuleInstruction>();
        ASSERT_TRUE(res.has_value());
        EXPECT_EQ(res->m_opcode.m_node, "i32");
        ASSERT_EQ(res->m_operands.size(), 1);
        EXPECT_EQ(res->m_operands[0].m_kind, DSL::Ast::LegalizeRuleDef::RuleOperandKind::SsaRegister);
        EXPECT_EQ(res->m_operands[0].m_name.m_node, "dst");
    }

    {
        std::string test = "ADD i32:$dst, $lhs, imm(i32):$c, 42, log2($shift);";
        ParseContext ctx = createParseContextFromBuff("test_inst_mixed", test);
        auto res =
                ctx.parse<DSL::Parser::LegalizeRuleDef::RuleInstruction, DSL::Ast::LegalizeRuleDef::RuleInstruction>();
        ASSERT_TRUE(res.has_value());
        EXPECT_EQ(res->m_opcode.m_node, "ADD");
        ASSERT_EQ(res->m_operands.size(), 5);
        EXPECT_EQ(res->m_operands[0].m_kind, DSL::Ast::LegalizeRuleDef::RuleOperandKind::SsaRegister);
        EXPECT_EQ(res->m_operands[1].m_kind, DSL::Ast::LegalizeRuleDef::RuleOperandKind::SsaRegister);
        EXPECT_EQ(res->m_operands[2].m_kind, DSL::Ast::LegalizeRuleDef::RuleOperandKind::ImmediateSymbol);
        EXPECT_EQ(res->m_operands[3].m_kind, DSL::Ast::LegalizeRuleDef::RuleOperandKind::ImmediateLiteral);
        EXPECT_EQ(res->m_operands[4].m_kind, DSL::Ast::LegalizeRuleDef::RuleOperandKind::CustomTransform);
    }
}

// ============================================================================
// 3. RuleWhen (Predicate Guard) Tests
// ============================================================================

TEST_F(LegalizeRuleDefLangTest, TestRuleWhenPredicates)
{
    {
        std::string test = "alwaysTrue();";
        ParseContext ctx = createParseContextFromBuff("test_pred_zero_arg", test);
        auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleWhen, DSL::Ast::LegalizeRuleDef::RuleWhen>();
        ASSERT_TRUE(res.has_value());
        EXPECT_EQ(res->m_predicateName.m_node, "alwaysTrue");
        EXPECT_TRUE(res->m_arguments.empty());
    }

    {
        std::string test = "checkRange($c, -128, 127, i8);";
        ParseContext ctx = createParseContextFromBuff("test_pred_multi_arg", test);
        auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::RuleWhen, DSL::Ast::LegalizeRuleDef::RuleWhen>();
        ASSERT_TRUE(res.has_value());
        EXPECT_EQ(res->m_predicateName.m_node, "checkRange");
        ASSERT_EQ(res->m_arguments.size(), 4);

        ASSERT_TRUE(std::holds_alternative<DSL::Ast::Common::Identifier>(res->m_arguments[0]));
        EXPECT_EQ(std::get<DSL::Ast::Common::Identifier>(res->m_arguments[0]).m_node, "c");

        ASSERT_TRUE(std::holds_alternative<DSL::Ast::Common::IntegerLiteral>(res->m_arguments[1]));
        EXPECT_EQ(std::get<DSL::Ast::Common::IntegerLiteral>(res->m_arguments[1]).m_node, -128);

        ASSERT_TRUE(std::holds_alternative<DSL::Ast::Common::IntegerLiteral>(res->m_arguments[2]));
        EXPECT_EQ(std::get<DSL::Ast::Common::IntegerLiteral>(res->m_arguments[2]).m_node, 127);

        ASSERT_TRUE(std::holds_alternative<DSL::Ast::Common::Identifier>(res->m_arguments[3]));
        EXPECT_EQ(std::get<DSL::Ast::Common::Identifier>(res->m_arguments[3]).m_node, "i8");
    }
}

// ============================================================================
// 4. LegalizeRule Tests (Ordering, Blocks & Synonyms)
// ============================================================================

TEST_F(LegalizeRuleDefLangTest, TestCompleteLegalizeRuleStandardOrder)
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
    emit {
        SAR i32:$dst, i32:$lhs, log2($c);
    };
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test_rule_standard_order", test);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::LegalizeRule, DSL::Ast::LegalizeRuleDef::LegalizeRule>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_ruleName.m_node, "SDivPow2");

    ASSERT_EQ(res->m_matchClauses.size(), 1);
    EXPECT_EQ(res->m_matchClauses[0].m_opcode.m_node, "SDIV");
    ASSERT_EQ(res->m_matchClauses[0].m_operands.size(), 3);
    EXPECT_EQ(res->m_matchClauses[0].m_operands[0].m_name.m_node, "dst");
    EXPECT_EQ(res->m_matchClauses[0].m_operands[2].m_kind, DSL::Ast::LegalizeRuleDef::RuleOperandKind::ImmediateSymbol);

    ASSERT_EQ(res->m_whenClauses.size(), 2);
    EXPECT_EQ(res->m_whenClauses[0].m_predicateName.m_node, "isPowTwo");
    EXPECT_EQ(res->m_whenClauses[1].m_predicateName.m_node, "isPositiveConst");

    ASSERT_EQ(res->m_emitClauses.size(), 1);
    EXPECT_EQ(res->m_emitClauses[0].m_opcode.m_node, "SAR");
    ASSERT_EQ(res->m_emitClauses[0].m_operands.size(), 3);
    EXPECT_EQ(res->m_emitClauses[0].m_operands[2].m_kind, DSL::Ast::LegalizeRuleDef::RuleOperandKind::CustomTransform);
    EXPECT_EQ(res->m_emitClauses[0].m_operands[2].m_name.m_node, "log2");
}

TEST_F(LegalizeRuleDefLangTest, TestPermutedBlocksAndExpandKeywordSynonym)
{
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
    ParseContext ctx = createParseContextFromBuff("test_rule_permuted_order", test);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::LegalizeRule, DSL::Ast::LegalizeRuleDef::LegalizeRule>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_ruleName.m_node, "PermutedOrder");
    ASSERT_EQ(res->m_matchClauses.size(), 1);
    ASSERT_EQ(res->m_whenClauses.size(), 1);
    ASSERT_EQ(res->m_emitClauses.size(), 1);
}

TEST_F(LegalizeRuleDefLangTest, TestRuleWithoutWhenBlock)
{
    std::string test = R"dsl(
rule NarrowAddi64 {
    match {
        ADD i64:$dst, i64:$lhs, i64:$rhs;
    };
    emit {
        UNMERGE_VALUES i32:$lhs_lo, i32:$lhs_hi, i64:$lhs;
        UNMERGE_VALUES i32:$rhs_lo, i32:$rhs_hi, i64:$rhs;
        UADDO i32:$dst_lo, i1:$carry, i32:$lhs_lo, i32:$rhs_lo;
        UADDE i32:$dst_hi, i1:$carry_out, i32:$lhs_hi, i32:$rhs_hi, i1:$carry;
        MERGE_VALUES i64:$dst, i32:$dst_lo, i32:$dst_hi;
    };
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test_rule_without_when", test);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::LegalizeRule, DSL::Ast::LegalizeRuleDef::LegalizeRule>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_ruleName.m_node, "NarrowAddi64");
    ASSERT_EQ(res->m_matchClauses.size(), 1);
    EXPECT_TRUE(res->m_whenClauses.empty());
    ASSERT_EQ(res->m_emitClauses.size(), 5);
}

// ============================================================================
// 5. Full File / Translation Unit Tests
// ============================================================================

TEST_F(LegalizeRuleDefLangTest, TestFullLegalizeRuleFile)
{
    std::string test = R"dsl(
rule SDivPow2 {
    match {
        SDIV i32:$dst, i32:$lhs, imm(i32):$c;
    };
    when {
        isPowTwo($c);
    };
    emit {
        SAR i32:$dst, i32:$lhs, log2($c);
    };
};

rule NarrowAddi64 {
    match {
        ADD i64:$dst, i64:$lhs, i64:$rhs;
    };
    expand {
        NOP;
    };
};
)dsl";

    ParseContext ctx = createParseContextFromBuff("test_full_legalize_file", test);

    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::LegalizeRuleFile, DSL::Ast::LegalizeRuleDef::LegalizeRuleFile>();
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->m_rules.size(), 2);
    EXPECT_EQ(res->m_rules[0].m_ruleName.m_node, "SDivPow2");
    EXPECT_EQ(res->m_rules[1].m_ruleName.m_node, "NarrowAddi64");
}

// ============================================================================
// 6. Syntax Error Tests
// ============================================================================

TEST_F(LegalizeRuleDefLangTest, TestMissingSemicolonInInstructionError)
{
    std::string test = R"dsl(
rule BadRule {
    match {
        ADD i32:$dst, i32:$lhs, i32:$rhs
    };
    emit {
        NOP;
    };
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test_err_missing_semi_inst", test);
    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::LegalizeRule, DSL::Ast::LegalizeRuleDef::LegalizeRule>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(LegalizeRuleDefLangTest, TestMissingSemicolonInWhenPredicateError)
{
    std::string test = R"dsl(
rule BadRule {
    match {
        ADD i32:$dst, i32:$lhs, imm:$c;
    };
    when {
        isPowTwo($c)
    };
    emit {
        NOP;
    };
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test_err_missing_semi_pred", test);
    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::LegalizeRule, DSL::Ast::LegalizeRuleDef::LegalizeRule>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(LegalizeRuleDefLangTest, TestUnclosedBraceInEmitBlockError)
{
    std::string test = R"dsl(
rule BadRule {
    match {
        NOP;
    };
    emit {
        NOP;
)dsl";
    ParseContext ctx = createParseContextFromBuff("test_err_unclosed_brace", test);
    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::LegalizeRule, DSL::Ast::LegalizeRuleDef::LegalizeRule>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(LegalizeRuleDefLangTest, TestInvalidBlockKeywordError)
{
    std::string test = R"dsl(
rule BadRule {
    transform {
        NOP;
    };
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test_err_invalid_block_keyword", test);
    auto res = ctx.parse<DSL::Parser::LegalizeRuleDef::LegalizeRule, DSL::Ast::LegalizeRuleDef::LegalizeRule>();
    EXPECT_FALSE(res.has_value());
}