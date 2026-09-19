#ifndef EZDSLLEXER_LEGALIZE_RULE_DEF_LANG_AST_H
#define EZDSLLEXER_LEGALIZE_RULE_DEF_LANG_AST_H

#include "Ast/CommonAstNodes.h"
#include "EzDslLexerCommon.h"

namespace DSL::Ast::LegalizeRuleDef
{

/**
 * Kind of operand appearing in a rewrite rule's match or expansion instruction.
 */
enum class RuleOperandKind : uint8_t
{
    SsaRegister,      // SSA virtual register (e.g., "$dst", "i32:$dst")
    ImmediateLiteral, // Concrete integer literal immediate (e.g., 0, 42, 0xFF)
    ImmediateSymbol,  // Bound symbolic immediate constant (e.g., "imm:$c", "imm(i32):$c")
    CustomTransform   // Compile-time transform hook (e.g., "log2($c)")
};

/**
 * One operand slot of a rule instruction, covering bound SSA variables, symbolic/literal
 * immediates, and compile-time transform calls.
 */
struct RuleInstructionOperand
{
    RuleOperandKind m_kind;                             // Operand form.
    Common::Identifier m_name;                          // Variable or transform function name
    std::optional<Common::Identifier> m_type;           // Base type or classifier ("i32", "imm", "GPR")
    std::optional<Common::Identifier> m_typeParam;      // Optional parameter ("i32" in "imm(i32):$c")
    std::optional<Common::IntegerLiteral> m_immLiteral; // Value if m_kind == ImmediateLiteral
    std::pmr::vector<Common::Identifier> m_callArgs;    // Variable arguments if m_kind == CustomTransform
};

/**
 * An instruction appearing in a rule's match pattern or expansion template.
 */
struct RuleInstruction
{
    Common::Identifier m_opcode;                         // Referenced generic IR opcode.
    std::pmr::vector<RuleInstructionOperand> m_operands; // Operand bindings.
};

/**
 * Argument to a `when` predicate: either a bound variable reference or an integer literal.
 */
using PredicateArg = std::variant<Common::Identifier, Common::IntegerLiteral>;

/**
 * A semantic guard predicate on a rewrite rule, e.g. `isPowerOfTwo($c)`.
 */
struct RuleWhen
{
    Common::Identifier m_predicateName;         // Predicate name.
    std::pmr::vector<PredicateArg> m_arguments; // Bound-variable/literal arguments.
};

/**
 * A complete IR-to-IR rewrite rule: match pattern, guard predicates, and expansion sequence.
 */
struct LegalizeRule
{
    Common::Identifier m_ruleName;                    // Rule name (also its symbol).
    std::pmr::vector<RuleInstruction> m_matchClauses; // Pattern the rule matches.
    std::pmr::vector<RuleWhen> m_whenClauses;         // Guards that must hold.
    std::pmr::vector<RuleInstruction> m_emitClauses;  // Instructions produced on a match.
};

/**
 * Root AST node for a parsed `.lrd` legalization rewrite rule file.
 */
struct LegalizeRuleFile
{
    std::pmr::vector<LegalizeRule> m_rules; // All declared rewrite rules.
};

} // namespace DSL::Ast::LegalizeRuleDef

#endif // EZDSLLEXER_LEGALIZE_RULE_DEF_LANG_AST_H