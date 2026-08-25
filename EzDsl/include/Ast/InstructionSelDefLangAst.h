#ifndef EZDSL_INST_SEL_DEF_LANG_AST_H
#define EZDSL_INST_SEL_DEF_LANG_AST_H

#include "Ast/CommonAstNodes.h"
#include "Ast/LegalizeRuleDefLangAst.h"
#include "EzDslCommon.h"

namespace DSL::Ast::InstSelDef
{

/**
 * Parameter declaration for an addressing mode aggregate in .isf files.
 *
 * Syntax:
 *   AddrModeParam := TypeName ('(' TypeParam ')')? ':' ParamName ( '=' DefaultValue )?
 *   TypeName      := Identifier
 *   TypeParam     := Identifier
 *   ParamName     := Identifier
 *   DefaultValue  := IntegerLiteral
 *
 * Examples:
 *   GPR:base
 *   simm(i12):offset = 0
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
 *
 * Syntax:
 *   AddrModeVariant := 'variant' VariantName '{' ( VariantBlock ';' )* '}'
 *   VariantBlock    := MatchBlock | WhenBlock
 *   MatchBlock      := 'match' '{' ( RuleInstruction )* '}'
 *   WhenBlock       := 'when' '{' ( RulePredicate )* '}'
 *
 * Example:
 *   variant RegOffset {
 *       match {
 *           ADD $addr, GPR:$base, simm(i12):$offset;
 *       };
 *   };
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
 * Syntax:
 *   AddrModeDef := 'addrmode' AddrModeName '(' ( AddrModeParam (',' AddrModeParam)* )? ')' '{' ( AddrModeVariant ';' )* '}'
 *   AddrModeName := Identifier
 *
 * Example:
 *   addrmode BaseOffset(GPR:base, simm(i12):offset = 0) {
 *       variant RegOffset {
 *           match { ADD $addr, GPR:$base, simm(i12):$offset; };
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
 * ISel pattern rule definition mapping generic IR match patterns to target instruction emit sequences.
 *
 * Syntax:
 *   ISelPattern  := 'pattern' PatternName '{' ( PatternBlock ';' )* '}'
 *   PatternBlock := MatchBlock | WhenBlock | EmitBlock | CostBlock
 *   MatchBlock   := 'match' '{' ( RuleInstruction )* '}'
 *   WhenBlock    := 'when' '{' ( RulePredicate )* '}'
 *   EmitBlock    := 'emit' '{' ( RuleInstruction )* '}'
 *   CostBlock    := 'cost' '(' IntegerLiteral ')'
 *
 * Example:
 *   pattern SelectAdd {
 *       match {
 *           ADD GPR:$dst, GPR:$lhs, GPR:$rhs;
 *       };
 *       emit {
 *           ADD GPR:$dst, GPR:$lhs, GPR:$rhs;
 *       };
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

/**
 * Root AST structure representing a parsed .isf (Instruction Selection Definition) file.
 *
 * Syntax:
 *   ISelDefFile := ( ( AddrModeDef | ISelPattern ) ';' )* EOF
 */
struct ISelDefFile
{
    std::pmr::vector<AddrModeDef> m_addrModes;
    std::pmr::vector<ISelPattern> m_patterns;
};

} // namespace DSL::Ast::InstSelDef

#endif // EZDSL_INST_SEL_DEF_LANG_AST_H