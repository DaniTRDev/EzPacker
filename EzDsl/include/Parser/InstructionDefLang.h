#ifndef EZDSL_INST_SEL_DEF_LANG_H
#define EZDSL_INST_SEL_DEF_LANG_H

#include "EzDslCommon.h"
#include "Ast/InstructionSelDefLang.h"
#include "Parser/CommonParsers.h"
#include "Parser/LegalizeRuleDefLang.h"

namespace DSL::Parser::InstSelDef
{
namespace dsl = ::lexy::dsl;

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
            [](Ast::Common::Identifier type, auto optTypeParam, Ast::Common::Identifier name, auto optDefVal)
            {
                std::optional<Ast::Common::Identifier> typeParam;
                if constexpr (std::is_same_v<std::decay_t<decltype(optTypeParam)>, Ast::Common::Identifier>)
                {
                    typeParam = std::move(optTypeParam);
                }

                std::optional<Ast::Common::IntegerLiteral> defVal;
                if constexpr (std::is_same_v<std::decay_t<decltype(optDefVal)>, Ast::Common::IntegerLiteral>)
                {
                    defVal = optDefVal;
                }

                return Ast::InstSelDef::AddrModeParam{ .m_typeOrClass = std::move(type),
                                                       .m_typeParam = std::move(typeParam),
                                                       .m_name = std::move(name),
                                                       .m_defaultValue = defVal };
            });
};

struct AddrModeParamList
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::parenthesized.list(dsl::p<AddrModeParam>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = lexy::as_list<std::pmr::vector<Ast::InstSelDef::AddrModeParam>>;
};

using VariantBlockClause = std::variant<LegalizeRuleDef::MatchClause, LegalizeRuleDef::WhenClause>;

struct VariantBlock
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = (LEXY_KEYWORD("match", dsl::identifier(dsl::ascii::alpha_underscore)) >>
                                  dsl::p<LegalizeRuleDef::MatchBlockBody>) |
            (LEXY_KEYWORD("when", dsl::identifier(dsl::ascii::alpha_underscore)) >>
             dsl::p<LegalizeRuleDef::WhenBlockBody>);

    static constexpr auto value = lexy::construct<VariantBlockClause>;
};

struct AddrModeVariantParser
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = LEXY_KEYWORD("variant", dsl::identifier(dsl::ascii::alpha_underscore)) >>
            (dsl::p<Common::Identifier> + dsl::curly_bracketed.list(dsl::p<VariantBlock> + dsl::lit_c<';'>));

    static constexpr auto
            value = lexy::as_list<std::pmr::vector<VariantBlockClause>> >>
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

struct AddrModeVariantList
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<AddrModeVariantParser> + dsl::lit_c<';'>);
    static constexpr auto value = lexy::as_list<std::pmr::vector<Ast::InstSelDef::AddrModeVariant>>;
};

struct AddrModeDefParser
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = LEXY_KEYWORD("addrmode", dsl::identifier(dsl::ascii::alpha_underscore)) >>
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

struct EmitClause
{
    std::pmr::vector<Ast::LegalizeRuleDef::RuleInstruction> instructions;
};

struct CostClause
{
    Ast::Common::IntegerLiteral cost;
};

using PatternBlockClause =
        std::variant<LegalizeRuleDef::MatchClause, LegalizeRuleDef::WhenClause, EmitClause, CostClause>;

struct EmitBlockBody
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<LegalizeRuleDef::RuleInstruction>);
    static constexpr auto value =
            lexy::as_list<std::pmr::vector<Ast::LegalizeRuleDef::RuleInstruction>> >> lexy::construct<EmitClause>;
};

struct CostBody
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::parenthesized(dsl::p<Common::IntegerLiteral>);
    static constexpr auto value =
            lexy::callback<CostClause>([](Ast::Common::IntegerLiteral lit) { return CostClause{ lit }; });
};

struct PatternBlock
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = (LEXY_KEYWORD("match", dsl::identifier(dsl::ascii::alpha_underscore)) >>
                                  dsl::p<LegalizeRuleDef::MatchBlockBody>) |
            (LEXY_KEYWORD("when", dsl::identifier(dsl::ascii::alpha_underscore)) >>
             dsl::p<LegalizeRuleDef::WhenBlockBody>) |
            (LEXY_KEYWORD("emit", dsl::identifier(dsl::ascii::alpha_underscore)) >> dsl::p<EmitBlockBody>) |
            (LEXY_KEYWORD("cost", dsl::identifier(dsl::ascii::alpha_underscore)) >> dsl::p<CostBody>);

    static constexpr auto value = lexy::construct<PatternBlockClause>;
};

struct ISelPatternParser
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = LEXY_KEYWORD("pattern", dsl::identifier(dsl::ascii::alpha_underscore)) >>
            (dsl::p<Common::Identifier> + dsl::curly_bracketed.list(dsl::p<PatternBlock> + dsl::lit_c<';'>));

    static constexpr auto
            value = lexy::as_list<std::pmr::vector<PatternBlockClause>> >>
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
            auto addrModeBranch = dsl::peek(LEXY_KEYWORD("addrmode", dsl::identifier(dsl::ascii::alpha_underscore))) >>
                    dsl::p<AddrModeDefParser>;
            auto patternBranch = dsl::peek(LEXY_KEYWORD("pattern", dsl::identifier(dsl::ascii::alpha_underscore))) >>
                    dsl::p<ISelPatternParser>;
            return (addrModeBranch | patternBranch) + dsl::lit_c<';'>;
        }();

        static constexpr auto value = lexy::callback<Entry>(
                [](Ast::InstSelDef::AddrModeDef addrMode) { return Entry{ std::move(addrMode) }; },
                [](Ast::InstSelDef::ISelPattern pattern) { return Entry{ std::move(pattern) }; });
    };

    static constexpr auto rule = dsl::terminator(dsl::eof).list(dsl::p<EntryParser>);

    static constexpr auto value = lexy::as_list<std::pmr::vector<Entry>> >>
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