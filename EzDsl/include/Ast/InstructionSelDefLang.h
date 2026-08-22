#ifndef EZDSL_INST_SEL_DEF_LANG_AST_H
#define EZDSL_INST_SEL_DEF_LANG_AST_H

#include "Ast/CommonAstNodes.h"
#include "Ast/LegalizeRuleDefLangAst.h"
#include "EzDslCommon.h"

namespace DSL::Ast::InstSelDef
{
/**
 * Represents a parameter in an AddrMode declaration with optional type parameter and default value.
 *
 * Examples:
 *   - "GPR:base"               -> m_typeOrClass="GPR",  m_typeParam=nullopt, m_name="base",   m_defaultValue=nullopt
 *   - "imm(i32):disp = 0"      -> m_typeOrClass="imm",  m_typeParam="i32",   m_name="disp",   m_defaultValue=0
 *   - "simm:disp = 0"          -> m_typeOrClass="simm",  m_typeParam=nullopt,   m_name="disp",   m_defaultValue=0
 */
struct AddrModeParam
{
    Common::Identifier m_typeOrClass;
    std::optional<Common::Identifier> m_typeParam;
    Common::Identifier m_name;
    std::optional<Common::IntegerLiteral> m_defaultValue;
};

/**
 * Individual matching variant inside an AddrMode aggregate.
 * Reuses `RuleInstruction` for the match block and `RulePredicate` for the when block.
 */
struct AddrModeVariant
{
    Common::Identifier m_variantName;
    std::pmr::vector<LegalizeRuleDef::RuleInstruction> m_matchPatterns;
    std::pmr::vector<LegalizeRuleDef::RulePredicate> m_predicates;
};

/**
 * Complete AddrMode aggregate definition.
 *
 * Example:
 *   addrmode AddrModeRegImm12(GPR:base, simm(i12):offset = 0) {
 *       variant OffsetAddr {
 *           match { G_PTR_ADD $addr, GPR:$base, simm(i12):$offset; };
 *           when  { hasOneUse($addr); immInRange($offset, -2048, 2047); };
 *       };
 *       variant BaseOnly {
 *           match { GPR:$base; };
 *       };
 *   };
 */
struct AddrModeDef
{
    Common::Identifier m_name;
    std::pmr::vector<AddrModeParam> m_parameters;
    std::pmr::vector<AddrModeVariant> m_variants;
};

/**
 * Complete pattern definition for instruction selection.
 * Reuses `RuleInstruction` for match and emit blocks, and `RulePredicate` for when guards.
 *
 * Example:
 *   pattern Select_LW {
 *       match { G_LOAD i32:$dst, AddrModeRegImm12($base, $offset); };
 *       when  { hasOneUse($base); };
 *       emit  { LW GPR:$dst, GPR:$base, $offset; };
 *       cost(1);
 *   };
 */
struct ISelPattern
{
    Common::Identifier m_patternName;
    std::pmr::vector<LegalizeRuleDef::RuleInstruction> m_matchPatterns;
    std::pmr::vector<LegalizeRuleDef::RulePredicate> m_predicates;
    std::pmr::vector<LegalizeRuleDef::RuleInstruction> m_emitSequence;
    std::optional<Common::IntegerLiteral> m_cost;
};

// ============================================================================
// 3. Translation Unit Root Node
// ============================================================================

/**
 * Root AST node for an entire ISel definition file (.isf).
 */
struct ISelDefFile
{
    std::pmr::vector<AddrModeDef> m_addrModes;
    std::pmr::vector<ISelPattern> m_patterns;
};

} // namespace DSL::Ast::InstSelDef

#endif // EZDSL_INST_SEL_DEF_LANG_AST_H