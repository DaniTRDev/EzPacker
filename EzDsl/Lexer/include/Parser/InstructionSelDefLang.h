#ifndef EZDSL_INST_SEL_DEF_LANG_H
#define EZDSL_INST_SEL_DEF_LANG_H

#include "Ast/InstructionSelDefLangAst.h"
#include "EzDslCommon.h"
#include "Parser/CommonParsers.h"
#include "Parser/LegalizeRuleDefLang.h"

#include <variant>

namespace DSL::Parser::InstSelDef
{
namespace dsl = ::lexy::dsl;

/**
 * Lexy parser rule for an addressing mode formal parameter with optional default value.
 *
 * Syntax:
 *   AddrModeParam := TypeName ('(' TypeParam ')')? ':' Identifier ( '=' IntegerLiteral )?
 *
 * Examples:
 *   GPR:base
 *   simm(i12):offset = 0
 */
struct AddrModeParam
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = []
    {
        auto typeParam = dsl::parenthesized(dsl::p<Common::Identifier>);
        auto type_or_class = dsl::p<Common::Identifier> + dsl::opt(typeParam);
        auto colon = dsl::lit<':'>;
        auto name = dsl::p<Common::Identifier>;
        auto optDefault = dsl::opt(dsl::lit<'='> >> dsl::p<Common::IntegerLiteral>);
        return type_or_class + colon + name + optDefault;
    }();

    static constexpr auto value = lexy::callback<Ast::InstSelDef::AddrModeParam>(
            [](Ast::Common::Identifier type,
               Ast::Common::Identifier typeParam,
               Ast::Common::Identifier name,
               Ast::Common::IntegerLiteral defVal)
            {
                return Ast::InstSelDef::AddrModeParam{ .m_typeOrClass = std::move(type),
                                                       .m_typeParam = std::move(typeParam),
                                                       .m_name = std::move(name),
                                                       .m_defaultValue = defVal };
            },
            [](Ast::Common::Identifier type,
               Ast::Common::Identifier typeParam,
               Ast::Common::Identifier name,
               lexy::nullopt)
            {
                return Ast::InstSelDef::AddrModeParam{ .m_typeOrClass = std::move(type),
                                                       .m_typeParam = std::move(typeParam),
                                                       .m_name = std::move(name),
                                                       .m_defaultValue = std::nullopt };
            },
            [](Ast::Common::Identifier type,
               lexy::nullopt,
               Ast::Common::Identifier name,
               Ast::Common::IntegerLiteral defVal)
            {
                return Ast::InstSelDef::AddrModeParam{ .m_typeOrClass = std::move(type),
                                                       .m_typeParam = std::nullopt,
                                                       .m_name = std::move(name),
                                                       .m_defaultValue = defVal };
            },
            [](Ast::Common::Identifier type, lexy::nullopt, Ast::Common::Identifier name, lexy::nullopt)
            {
                return Ast::InstSelDef::AddrModeParam{ .m_typeOrClass = std::move(type),
                                                       .m_typeParam = std::nullopt,
                                                       .m_name = std::move(name),
                                                       .m_defaultValue = std::nullopt };
            });
};

/**
 * Lexy parser rule for a comma-separated parenthesized list of AddrMode parameters.
 *
 * Syntax:
 *   AddrModeParamList := '(' ( AddrModeParam (',' AddrModeParam)* )? ')'
 */
struct AddrModeParamList
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::parenthesized.list(dsl::p<AddrModeParam>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::InstSelDef::AddrModeParam>>;
};

using VariantBlockClause = std::variant<LegalizeRuleDef::MatchClause, LegalizeRuleDef::WhenClause>;

/**
 * Lexy parser rule for addressing mode variant clauses (match or when blocks).
 */
struct VariantBlock
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = (Common::Keyword<"match">::rule >> dsl::p<LegalizeRuleDef::MatchBlockBody>) |
            (Common::Keyword<"when">::rule >> dsl::p<LegalizeRuleDef::WhenBlockBody>);

    static constexpr auto value = lexy::construct<VariantBlockClause>;
};

/**
 * Lexy parser rule for an individual addressing mode variant declaration.
 *
 * Syntax:
 *   AddrModeVariantParser := 'variant' Identifier '{' ( VariantBlock ';' )* '}'
 *
 * Example:
 *   variant RegOffset {
 *       match { ADD $addr, GPR:$base, simm(i12):$offset; };
 *   };
 */
struct AddrModeVariantParser
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = Common::Keyword<"variant">::rule >>
            (dsl::p<Common::Identifier> + dsl::curly_bracketed.list(dsl::p<VariantBlock> + dsl::lit_c<';'>));

    static constexpr auto
            value = Common::PmrAsList<std::pmr::vector<VariantBlockClause>> >>
            lexy::callback<Ast::InstSelDef::AddrModeVariant>(
                            [](Ast::Common::Identifier name, std::pmr::vector<VariantBlockClause> clauses)
                            {
                                Ast::InstSelDef::AddrModeVariant variant;
                                variant.m_variantName = std::move(name);

                                for (auto &clause : clauses)
                                {
                                    std::visit(
                                            [&](auto &&val)
                                            {
                                                using T = std::decay_t<decltype(val)>;
                                                if constexpr (std::is_same_v<T, LegalizeRuleDef::MatchClause>)
                                                    variant.m_matchPatterns = std::move(val.instructions);
                                                else if constexpr (std::is_same_v<T, LegalizeRuleDef::WhenClause>)
                                                    variant.m_predicates = std::move(val.predicates);
                                            },
                                            clause);
                                }
                                return variant;
                            });
};

/**
 * Lexy parser rule for a list of addressing mode variants.
 */
struct AddrModeVariantList
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<AddrModeVariantParser> + dsl::lit_c<';'>);
    static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::InstSelDef::AddrModeVariant>>;
};

/**
 * Lexy parser rule for an addressing mode aggregate definition.
 *
 * Syntax:
 *   AddrModeDefParser := 'addrmode' Identifier AddrModeParamList AddrModeVariantList
 *
 * Example:
 *   addrmode BaseOffset(GPR:base, simm(i12):offset = 0) {
 *       variant RegOffset { match { ADD $addr, GPR:$base, simm(i12):$offset; }; };
 *   };
 */
struct AddrModeDefParser
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = Common::Keyword<"addrmode">::rule >>
            (dsl::p<Common::Identifier> + dsl::p<AddrModeParamList> + dsl::p<AddrModeVariantList>);

    static constexpr auto value = lexy::callback<Ast::InstSelDef::AddrModeDef>(
            [](Ast::Common::Identifier name,
               std::pmr::vector<Ast::InstSelDef::AddrModeParam> params,
               std::pmr::vector<Ast::InstSelDef::AddrModeVariant> variants)
            {
                return Ast::InstSelDef::AddrModeDef{ .m_name = std::move(name),
                                                     .m_parameters = std::move(params),
                                                     .m_variants = std::move(variants) };
            });
};

/**
 * Clause container for the emit { ... } sequence.
 */
struct EmitClause
{
    std::pmr::vector<Ast::LegalizeRuleDef::RuleInstruction> instructions;
};

/**
 * Clause container for the cost(N) metric.
 */
struct CostClause
{
    Ast::Common::IntegerLiteral cost;
};

using PatternBlockClause =
        std::variant<LegalizeRuleDef::MatchClause, LegalizeRuleDef::WhenClause, EmitClause, CostClause>;

/**
 * Lexy parser rule for emit { ... } blocks in ISel patterns.
 *
 * Syntax:
 *   EmitBlockBody := '{' ( RuleInstruction )* '}'
 */
struct EmitBlockBody
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<LegalizeRuleDef::RuleInstruction>);
    static constexpr auto value =
            Common::PmrAsList<std::pmr::vector<Ast::LegalizeRuleDef::RuleInstruction>> >> lexy::construct<EmitClause>;
};

/**
 * Lexy parser rule for cost(N) clauses in ISel patterns.
 *
 * Syntax:
 *   CostBody := '(' IntegerLiteral ')'
 */
struct CostBody
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::parenthesized(dsl::p<Common::IntegerLiteral>);
    static constexpr auto value =
            lexy::callback<CostClause>([](Ast::Common::IntegerLiteral lit) { return CostClause{ lit }; });
};

/**
 * Lexy parser rule for ISel pattern block items (match, when, emit, cost).
 */
struct PatternBlock
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = (Common::Keyword<"match">::rule >> dsl::p<LegalizeRuleDef::MatchBlockBody>) |
            (Common::Keyword<"when">::rule >> dsl::p<LegalizeRuleDef::WhenBlockBody>) |
            (Common::Keyword<"emit">::rule >> dsl::p<EmitBlockBody>) |
            (Common::Keyword<"cost">::rule >> dsl::p<CostBody>);

    static constexpr auto value = lexy::construct<PatternBlockClause>;
};

/**
 * Lexy parser rule for a complete ISel pattern rule.
 *
 * Syntax:
 *   ISelPatternParser := 'pattern' Identifier '{' ( PatternBlock ';' )* '}'
 *
 * Example:
 *   pattern SelectAdd {
 *       match { ADD GPR:$dst, GPR:$lhs, GPR:$rhs; };
 *       emit { ADD GPR:$dst, GPR:$lhs, GPR:$rhs; };
 *       cost(1);
 *   };
 */
struct ISelPatternParser
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = Common::Keyword<"pattern">::rule >>
            (dsl::p<Common::Identifier> + dsl::curly_bracketed.list(dsl::p<PatternBlock> + dsl::lit_c<';'>));

    static constexpr auto
            value = Common::PmrAsList<std::pmr::vector<PatternBlockClause>> >>
            lexy::callback<Ast::InstSelDef::ISelPattern>(
                            [](Ast::Common::Identifier name, std::pmr::vector<PatternBlockClause> clauses)
                            {
                                Ast::InstSelDef::ISelPattern pat;
                                pat.m_patternName = std::move(name);

                                for (auto &clause : clauses)
                                {
                                    std::visit(
                                            [&](auto &&val)
                                            {
                                                using T = std::decay_t<decltype(val)>;
                                                if constexpr (std::is_same_v<T, LegalizeRuleDef::MatchClause>)
                                                    pat.m_matchPatterns = std::move(val.instructions);
                                                else if constexpr (std::is_same_v<T, LegalizeRuleDef::WhenClause>)
                                                    pat.m_predicates = std::move(val.predicates);
                                                else if constexpr (std::is_same_v<T, EmitClause>)
                                                    pat.m_emitSequence = std::move(val.instructions);
                                                else if constexpr (std::is_same_v<T, CostClause>)
                                                    pat.m_cost = val.cost;
                                            },
                                            clause);
                                }
                                return pat;
                            });
};

/**
 * Top-level Lexy file parser for .isf instruction selection definition files.
 *
 * Syntax:
 *   ISelDefFileParser := ( ( AddrModeDefParser | ISelPatternParser ) ';' )* EOF
 */
struct ISelDefFileParser
{
    static constexpr auto whitespace = Common::Whitespace;

    struct Entry
    {
        std::variant<Ast::InstSelDef::AddrModeDef, Ast::InstSelDef::ISelPattern> decl;
    };

    struct EntryParser
    {
        static constexpr auto whitespace = Common::Whitespace;

        static constexpr auto rule = []
        {
            auto addrModeBranch = dsl::peek(Common::Keyword<"addrmode">::rule) >> dsl::p<AddrModeDefParser>;
            auto patternBranch = dsl::peek(Common::Keyword<"pattern">::rule) >> dsl::p<ISelPatternParser>;
            return (addrModeBranch | patternBranch) + dsl::lit_c<';'>;
        }();

        static constexpr auto value = lexy::callback<Entry>(
                [](Ast::InstSelDef::AddrModeDef addrMode) { return Entry{ std::move(addrMode) }; },
                [](Ast::InstSelDef::ISelPattern pattern) { return Entry{ std::move(pattern) }; });
    };

    static constexpr auto rule = dsl::terminator(dsl::eof).list(dsl::p<EntryParser>);

    static constexpr auto value = Common::PmrAsList<std::pmr::vector<Entry>> >>
            lexy::callback<Ast::InstSelDef::ISelDefFile>(
                                          [](std::pmr::vector<Entry> entries)
                                          {
                                              Ast::InstSelDef::ISelDefFile file;
                                              for (auto &entry : entries)
                                              {
                                                  if (std::holds_alternative<Ast::InstSelDef::AddrModeDef>(entry.decl))
                                                  {
                                                      file.m_addrModes.push_back(std::get<Ast::InstSelDef::AddrModeDef>(
                                                              std::move(entry.decl)));
                                                  }
                                                  else
                                                  {
                                                      file.m_patterns.push_back(std::get<Ast::InstSelDef::ISelPattern>(
                                                              std::move(entry.decl)));
                                                  }
                                              }
                                              return file;
                                          });
};

} // namespace DSL::Parser::InstSelDef

#endif // EZDSL_INST_SEL_DEF_LANG_H