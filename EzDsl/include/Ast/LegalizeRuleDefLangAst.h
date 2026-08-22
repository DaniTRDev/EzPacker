#ifndef EZDSL_LEGALIZE_RULE_DEF_LANG_AST_H
#define EZDSL_LEGALIZE_RULE_DEF_LANG_AST_H

#include "EzDslCommon.h"
#include "Ast/CommonAstNodes.h"

namespace DSL::Ast::LegalizeRuleDef
{
/**
 * Identifies the nature of an operand inside a pattern or expansion instruction.
 */
enum class OperandKind
{
    SsaRegister,      // An SSA virtual register (e.g., "$dst", "i32:$dst", "$src1").
    ImmediateLiteral, // A concrete integer literal immediate (e.g., 0, 42, 0xFF).
    ImmediateSymbol,  // A bound symbolic immediate constant (e.g., "imm:$c", "imm(i32):$c").
    CustomTransform   // A compile-time function transform (e.g., "log2($c)", "neg($imm)").
};

/**
 * Represents an operand within a pattern match or expansion IR instruction.
 *
 * Supported Syntax:
 * - "i32:$dst"     -> SSA register (m_type="i32", m_name="dst")
 * - "imm(i32):$c"  -> Immediate symbol (m_type="imm", m_typeParam="i32", m_name="c")
 * - "imm:$c"       -> Immediate symbol (m_type="imm", m_name="c")
 * - "$src"         -> Bare SSA register (m_name="src", m_type=std::nullopt)
 * - "42", "0xFF"   -> Immediate literal (m_immLiteral=42)
 * - "log2($shift)" -> Custom transform expression
 */
struct RuleOperand
{
    OperandKind m_kind = OperandKind::SsaRegister;
    Common::Identifier m_name;                          // Variable or transform function name.
    std::optional<Common::Identifier> m_type;           // Base type or classifier ("i32", "imm", "GPR").
    std::optional<Common::Identifier> m_typeParam;      // Optional parameter ("i32" in "imm(i32):$c").
    std::optional<Common::IntegerLiteral> m_immLiteral; // Value if m_kind == ImmediateLiteral.
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
    Common::Identifier m_opcode;              // Generic IR opcode name (e.g., "ADD", "SEXT").
    std::pmr::vector<RuleOperand> m_operands; // Dest and source operand(s). This is known by MIR instr metadata.
};

// Represents an argument to a predicate: either an identifier ($c, Zba) or an integer literal (-2048, 2047).
using PredicateArg = std::variant<Common::Identifier, Common::IntegerLiteral>;

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
    Common::Identifier m_predicateName;         // Name of the predicate function / hook.
    std::pmr::vector<PredicateArg> m_arguments; // Bound variables, subtarget features, or integer literals.
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
}; // namespace DSL::Ast::LegalizeRuleDef

#endif // EZDSL_LEGALIZE_RULE_DEF_LANG_AST_H
