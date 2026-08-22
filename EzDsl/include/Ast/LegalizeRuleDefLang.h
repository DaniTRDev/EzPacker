#ifndef EZDSL_LEGALIZE_RULE_DEF_LANG_H
#define EZDSL_LEGALIZE_RULE_DEF_LANG_H

#include "EzDslCommon.h"
#include "Ast/CommonAstNodes.h"

namespace DSL::Ast::LegalizeRuleDefLang
{
namespace DSL::Ast::LegalizeRuleDefLang
{
/**
 * Identifies the nature of an operand inside a pattern or expansion instruction.
 */
enum class OperandKind
{
    SsaRegister,      // An SSA virtual register (e.g., "$dst", "$src1").
    ImmediateLiteral, // A concrete integer literal immediate (e.g., 0, 42, 0xFF).
    ImmediateSymbol,  // A bound symbolic immediate constant (e.g., "$c:imm").
    CustomTransform   // A compile-time function transform (e.g., "log2($c)", "neg($imm)").
};

/**
 * Represents an operand within a pattern match or expansion IR instruction.
 *
 * Examples:
 * - "i32:$dst"     -> SSA register named "dst", explicit type "i32"
 * - "$src"         -> SSA register named "src", type inferred from match context
 * - "$c:imm"       -> Symbolic immediate operand named "c"
 * - "0x20"         -> Literal immediate constant
 * - "log2($shift)" -> Custom transform expression applied to "$shift"
 */
struct RuleOperand
{
    OperandKind m_kind = OperandKind::SsaRegister;
    Common::Identifier m_name;                          // Variable or transform function name.
    std::optional<Common::Identifier> m_type;           // Optional type binding (e.g., "i8", "i32", "imm").
    std::optional<Common::IntegerLiteral> m_immLiteral; // Concrete value if m_kind == ImmediateLiteral.
    std::pmr::vector<Common::Identifier> m_callArgs;    // Arguments if m_kind == CustomTransform.
};

/**
 * Represents a generic IR instruction in either a `match` or `expand` block.
 *
 * Supports single-definition instructions (e.g., arithmetic, conversions) as well as
 * void instructions without a destination operand (e.g., memory stores, branches).
 *
 * Syntax Examples:
 * - "ADD i32:$dst, i32:$lhs, i32:$rhs;"
 * - "SEXT i32:$dst, i8:$src;"
 * - "STORE i32:$val, ptr:$ptr;"
 */
struct RuleInstruction
{
    std::pmr::vector<RuleOperand> m_defs; // Target / destination operand(s) on the LHS.
    Common::Identifier m_opcode;          // Generic IR opcode name (e.g., "ADD", "SEXT").
    std::pmr::vector<RuleOperand> m_uses; // Source operand(s) on the RHS.
};

/**
 * A semantic predicate check defined in a `when { ... }` guard clause.
 *
 * Used to restrict rewrite rule application beyond pure type matching
 * (e.g., validating constant ranges, alignment, or power-of-two properties).
 *
 * Syntax Examples:
 * - "isPowTwo($c);"
 * - "isPositiveConst($imm);"
 * - "hasOneUse($val);"
 */
struct RulePredicate
{
    Common::Identifier m_predicateName;               // Name of the predicate function / hook.
    std::pmr::vector<Common::Identifier> m_arguments; // Bound SSA or immediate variables passed to the check.
};

/**
 * Declarative IR-to-IR lowering and decomposition rule.
 *
 * Specifies how to match an illegal or un-lowered generic IR instruction pattern,
 * validate preconditions via predicates, and expand it into a sequence of equivalent
 * legal generic IR instructions.
 *
 * Syntax Example:
 * @code
 * rule NarrowAddi64 {
 *     match {
 *         ADD i64:$dst, i64:$lhs, i64:$rhs;
 *     }
 *     when {
 *         // Optional guard conditions / compile-time predicates
 *     }
 *     expand {
 *         // 1. Decompose 64-bit inputs into 32-bit halves
 *         UNMERGE_VALUES i32:$lhs_lo, i32:$lhs_hi, i64:$lhs;
 *         UNMERGE_VALUES i32:$rhs_lo, i32:$rhs_hi, i64:$rhs;
 *
 *         // 2. Perform 32-bit addition with explicit SSA carry propagation
 *         UADDO i32:$dst_lo, i1:$carry, i32:$lhs_lo, i32:$rhs_lo;
 *         UADDE i32:$dst_hi, i1:$carry_out, i32:$lhs_hi, i32:$rhs_hi, i1:$carry;
 *
 *         // 3. Reconstruct the 64-bit result
 *         MERGE_VALUES i64:$dst, i32:$dst_lo, i32:$dst_hi;
 *     }
 * };
 * @endcode
 */
struct LegalizeRewriteRule
{
    Common::Identifier m_ruleName;                         // Unique identifier for the rewrite rule.
    std::pmr::vector<RuleInstruction> m_matchPatterns;     // Pattern instruction(s) to search for in the IR.
    std::pmr::vector<RulePredicate> m_predicates;          // Guard conditions in the `when` clause.
    std::pmr::vector<RuleInstruction> m_expansionSequence; // Replacement sequence in the `expand` clause.
};

/**
 * Root AST node for a legalization rewrite rule definition file.
 *
 * Contains imported rule files and the set of declarative IR-to-IR expansion rules.
 *
 * Syntax Example:
 * @code
 * rule LowerSDivi8 { ... };
 * rule NarrowAddi64 { ... };
 * @endcode
 */
struct TargetLegalizeRuleDef
{
    std::pmr::vector<LegalizeRewriteRule> m_rules; // Collection of declared rewrite rules.
};
}; // namespace DSL::Ast::LegalizeRuleDefLang

#endif // EZDSL_LEGALIZE_RULE_DEF_LANG_H
