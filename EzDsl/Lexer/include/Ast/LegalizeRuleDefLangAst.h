#ifndef EZDSL_LEGALIZE_RULE_DEF_LANG_AST_H
#define EZDSL_LEGALIZE_RULE_DEF_LANG_AST_H

#include "Ast/CommonAstNodes.h"
#include "EzDslCommon.h"

#include <optional>
#include <variant>
#include <vector>

namespace DSL::Ast::LegalizeRuleDef
{

/**
 * Kind of operand appearing within rewrite rule patterns.
 */
enum class OperandKind : uint8_t
{
    SsaRegister,      // SSA virtual register (e.g., "$dst", "i32:$dst")
    ImmediateLiteral, // Concrete integer literal immediate (e.g., 0, 42, 0xFF)
    ImmediateSymbol,  // Bound symbolic immediate constant (e.g., "imm:$c", "imm(i32):$c")
    CustomTransform   // Compile-time transform hook (e.g., "log2($c)")
};

/**
 * Instruction operand inside a rewrite rule pattern match or expansion block.
 *
 * Syntax:
 *   RuleOperand := CustomTransform | TypedPrefixSsa | BareSsa | LiteralOperand
 *   CustomTransform   := FuncName '(' SsaVar (',' SsaVar)* ')'
 *   TypedPrefixSsa    := TypeName ('(' TypeParam ')')? ':' SsaVar
 *   BareSsa           := SsaVar
 *   SsaVar            := '$' Identifier
 *   LiteralOperand    := IntegerLiteral
 *
 * Examples:
 *   $dst
 *   i32:$lhs
 *   simm(i12):$imm
 *   42
 *   log2($val)
 */
struct RuleOperand
{
    OperandKind m_kind;
    Common::Identifier m_name;                          // Variable or transform function name
    std::optional<Common::Identifier> m_type;           // Base type or classifier ("i32", "imm", "GPR")
    std::optional<Common::Identifier> m_typeParam;      // Optional parameter ("i32" in "imm(i32):$c")
    std::optional<Common::IntegerLiteral> m_immLiteral; // Value if m_kind == ImmediateLiteral
    std::pmr::vector<Common::Identifier> m_callArgs;    // Variable arguments if m_kind == CustomTransform
};

/**
 * Generic IR instruction statement within rewrite patterns or expansions.
 *
 * Syntax:
 *   RuleInstruction := OpcodeName ( RuleOperand (',' RuleOperand)* )? ';' | SingleOperandStatement
 *   SingleOperandStatement := RuleOperand ';'
 *
 * Examples:
 *   ADD $dst, $lhs, $rhs;
 *   RET $val;
 */
struct RuleInstruction
{
    Common::Identifier m_opcode;
    std::pmr::vector<RuleOperand> m_operands;
};

using PredicateArg = std::variant<Common::Identifier, Common::IntegerLiteral>;

/**
 * Semantic guard predicate evaluated inside a `when { ... }` block.
 *
 * Syntax:
 *   RulePredicate := PredicateName '(' PredicateArg (',' PredicateArg)* ')' ';'
 *   PredicateArg  := '$' Identifier | IntegerLiteral | Identifier
 *
 * Examples:
 *   is_power_of_two($c);
 *   fits_in_simm12($imm);
 */
struct RulePredicate
{
    Common::Identifier m_predicateName;
    std::pmr::vector<PredicateArg> m_arguments;
};

/**
 * Complete IR-to-IR rewrite rule AST node.
 *
 * Syntax:
 *   LegalizeRewriteRule := 'rule' RuleName '{' ( RuleBlock ';' )* '}'
 *   RuleBlock := MatchBlock | WhenBlock | ExpandBlock
 *   MatchBlock  := 'match' '{' ( RuleInstruction )* '}'
 *   WhenBlock   := 'when' '{' ( RulePredicate )* '}'
 *   ExpandBlock := 'expand' '{' ( RuleInstruction )* '}'
 *
 * Example:
 *   rule LowerAddImm {
 *       match {
 *           ADD $dst, $src, imm(i32):$c;
 *       };
 *       when {
 *           is_simm12($c);
 *       };
 *       expand {
 *           ADDI $dst, $src, $c;
 *       };
 *   };
 */
struct LegalizeRewriteRule
{
    Common::Identifier m_ruleName;
    std::pmr::vector<RuleInstruction> m_matchPatterns;
    std::pmr::vector<RulePredicate> m_predicates;
    std::pmr::vector<RuleInstruction> m_expansionSequence;
};

/**
 * Root AST structure representing a parsed .lrd (Legalize Rule Definition) file.
 *
 * Syntax:
 *   TargetLegalizeRuleDef := ( LegalizeRewriteRule ';' )* EOF
 */
struct TargetLegalizeRuleDef
{
    std::pmr::vector<LegalizeRewriteRule> m_rules;
};

} // namespace DSL::Ast::LegalizeRuleDef

#endif // EZDSL_LEGALIZE_RULE_DEF_LANG_AST_H