#ifndef EZDSL_INST_SEL_DEF_LANG_AST_H
#define EZDSL_INST_SEL_DEF_LANG_AST_H

#include "Ast/CommonAstNodes.h"
#include "Ast/LegalizeRuleDefLangAst.h"
#include "EzDslCommon.h"

namespace DSL::Ast::InstSelDef
{

/**
 * Parameter declaration for an addressing mode aggregate.
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
 */
struct AddrModeVariant
{
    Common::Identifier m_variantName;
    std::pmr::vector<LegalizeRuleDef::RuleInstruction> m_matchPatterns;
    std::pmr::vector<LegalizeRuleDef::RulePredicate> m_predicates;
};

/**
 * Complete AddrMode aggregate definition.
 */
struct AddrModeDef
{
    Common::Identifier m_name;
    std::pmr::vector<AddrModeParam> m_parameters;
    std::pmr::vector<AddrModeVariant> m_variants;
};

/**
 * ISel pattern rule definition.
 */
struct ISelPattern
{
    Common::Identifier m_patternName;
    std::pmr::vector<LegalizeRuleDef::RuleInstruction> m_matchPatterns;
    std::pmr::vector<LegalizeRuleDef::RulePredicate> m_predicates;
    std::pmr::vector<LegalizeRuleDef::RuleInstruction> m_emitSequence;
    std::optional<Common::IntegerLiteral> m_cost;
};

struct ISelDefFile
{
    std::pmr::vector<AddrModeDef> m_addrModes;
    std::pmr::vector<ISelPattern> m_patterns;
};

} // namespace DSL::Ast::InstSelDef

#endif // EZDSL_INST_SEL_DEF_LANG_AST_H