#ifndef EZDSL_LEGALIZE_RULE_DEF_LANG_AST_H
#define EZDSL_LEGALIZE_RULE_DEF_LANG_AST_H

#include "Ast/CommonAstNodes.h"
#include "EzDslCommon.h"

#include <optional>
#include <variant>
#include <vector>

namespace DSL::Ast::LegalizeRuleDef
{

enum class OperandKind : uint8_t
{
    SsaRegister,      // SSA virtual register (e.g., "$dst", "i32:$dst")
    ImmediateLiteral, // Concrete integer literal immediate (e.g., 0, 42, 0xFF)
    ImmediateSymbol,  // Bound symbolic immediate constant (e.g., "imm:$c", "imm(i32):$c")
    CustomTransform   // Compile-time transform hook (e.g., "log2($c)")
};

/**
 * Instruction operand inside a pattern match or expansion block.
 */
struct RuleOperand
{
    OperandKind m_kind = OperandKind::SsaRegister;
    Common::Identifier m_name;                          // Variable or transform function name
    std::optional<Common::Identifier> m_type;           // Base type or classifier ("i32", "imm", "GPR")
    std::optional<Common::Identifier> m_typeParam;      // Optional parameter ("i32" in "imm(i32):$c")
    std::optional<Common::IntegerLiteral> m_immLiteral; // Value if m_kind == ImmediateLiteral
    std::pmr::vector<Common::Identifier> m_callArgs;    // Variable arguments if m_kind == CustomTransform
};

/**
 * Generic IR instruction statement.
 */
struct RuleInstruction
{
    Common::Identifier m_opcode;              // Opcode name (e.g., "ADD", "SEXT")
    std::pmr::vector<RuleOperand> m_operands; // Destination and source operands
};

using PredicateArg = std::variant<Common::Identifier, Common::IntegerLiteral>;

/**
 * Semantic guard check in a `when { ... }` block.
 */
struct RulePredicate
{
    Common::Identifier m_predicateName;         // Name of the predicate function / hook
    std::pmr::vector<PredicateArg> m_arguments; // Bound variables or integer constants
};

/**
 * IR-to-IR expansion rule definition.
 */
struct LegalizeRewriteRule
{
    Common::Identifier m_ruleName;
    std::pmr::vector<RuleInstruction> m_matchPatterns;
    std::pmr::vector<RulePredicate> m_predicates;
    std::pmr::vector<RuleInstruction> m_expansionSequence;
};

struct TargetLegalizeRuleDef
{
    std::pmr::vector<LegalizeRewriteRule> m_rules;
};

} // namespace DSL::Ast::LegalizeRuleDef

#endif // EZDSL_LEGALIZE_RULE_DEF_LANG_AST_H