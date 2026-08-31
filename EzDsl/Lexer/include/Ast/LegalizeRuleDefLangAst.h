#ifndef EZDSLexer_LEGALIZE_RULE_DEF_LANG_AST_H
#define EZDSLexer_LEGALIZE_RULE_DEF_LANG_AST_H

#include "Ast/CommonAstNodes.h"
#include "EzDslLexerCommon.h"

namespace DSL::Ast::LegalizeRuleDef
{

enum class RuleOperandKind : uint8_t
{
    SsaRegister,      // SSA virtual register (e.g., "$dst", "i32:$dst")
    ImmediateLiteral, // Concrete integer literal immediate (e.g., 0, 42, 0xFF)
    ImmediateSymbol,  // Bound symbolic immediate constant (e.g., "imm:$c", "imm(i32):$c")
    CustomTransform   // Compile-time transform hook (e.g., "log2($c)")
};

struct RuleInstructionOperand
{
    RuleOperandKind m_kind;
    Common::Identifier m_name;                          // Variable or transform function name
    std::optional<Common::Identifier> m_type;           // Base type or classifier ("i32", "imm", "GPR")
    std::optional<Common::Identifier> m_typeParam;      // Optional parameter ("i32" in "imm(i32):$c")
    std::optional<Common::IntegerLiteral> m_immLiteral; // Value if m_kind == ImmediateLiteral
    std::pmr::vector<Common::Identifier> m_callArgs;    // Variable arguments if m_kind == CustomTransform
};

struct RuleInstruction
{
    Common::Identifier m_opcode;
    std::pmr::vector<RuleInstructionOperand> m_operands;
};

using PredicateArg = std::variant<Common::Identifier, Common::IntegerLiteral>;

struct RuleWhen
{
    Common::Identifier m_predicateName;
    std::pmr::vector<PredicateArg> m_arguments;
};

struct LegalizeRule
{
    Common::Identifier m_ruleName;
    std::pmr::vector<RuleInstruction> m_matchClauses;
    std::pmr::vector<RuleWhen> m_whenClauses;
    std::pmr::vector<RuleInstruction> m_emitClauses;
};

struct LegalizeRuleFile
{
    std::pmr::vector<LegalizeRule> m_rules;
};

} // namespace DSL::Ast::LegalizeRuleDef

#endif // EZDSLexer_LEGALIZE_RULE_DEF_LANG_AST_H