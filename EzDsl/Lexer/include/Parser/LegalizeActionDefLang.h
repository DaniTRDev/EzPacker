#ifndef EZDSL_LEGALIZE_ACTION_LANG_H
#define EZDSL_LEGALIZE_ACTION_LANG_H

#include "Ast/LegalizeActionDefLangAst.h"
#include "EzDslLexerCommon.h"
#include "Parser/CommonParsers.h"

namespace DSL::Parser::LegalizeActionDef
{
namespace dsl = ::lexy::dsl;

struct TypeConstraint
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule =
            dsl::p<Common::Identifier> + dsl::opt(dsl::lit_c<':'> >> dsl::p<Common::IntegerLiteral>);

    static constexpr auto value = lexy::callback<Ast::LegalizeActionDef::TypeConstraint>(
            [](Ast::Common::Identifier typeName, auto index)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(index)>, Ast::Common::IntegerLiteral>)
                {
                    return Ast::LegalizeActionDef::TypeConstraint{ std::move(typeName), std::move(index) };
                }

                return Ast::LegalizeActionDef::TypeConstraint{ std::move(typeName), std::nullopt };
            });
};

struct LegalizationClauseKind
{
    static constexpr auto KindTable = lexy::symbol_table<Ast::LegalizeActionDef::LegalizeActionKind>
        .map(LEXY_LIT("LEGAL"), Ast::LegalizeActionDef::LegalizeActionKind::Legal)
        .map(LEXY_LIT("WIDENS"), Ast::LegalizeActionDef::LegalizeActionKind::WidenScalar)
        .map(LEXY_LIT("NARROWS"), Ast::LegalizeActionDef::LegalizeActionKind::NarrowScalar)
        .map(LEXY_LIT("LIBCALL"), Ast::LegalizeActionDef::LegalizeActionKind::Libcall)
        .map(LEXY_LIT("CUSTOM"), Ast::LegalizeActionDef::LegalizeActionKind::Custom)
        .map(LEXY_LIT("BITCAST"), Ast::LegalizeActionDef::LegalizeActionKind::Bitcast)
        .map(LEXY_LIT("UNSUPPORTED"), Ast::LegalizeActionDef::LegalizeActionKind::Unsupported);

    static constexpr auto rule = dsl::symbol<KindTable>(dsl::identifier(dsl::ascii::alpha_underscore));
    static constexpr auto value = lexy::forward<Ast::LegalizeActionDef::LegalizeActionKind>;
};

struct LegalizationClause
{
    static constexpr auto whitespace = Common::Whitespace;

    using TargetVariant = std::variant<std::pmr::vector<Ast::Common::Identifier>, Ast::Common::StringLiteral>;

    struct TargetParser
    {
        static constexpr auto whitespace = Common::Whitespace;

        struct StringTarget
        {
            static constexpr auto rule = dsl::p<Common::StringLiteral>;
            static constexpr auto value = lexy::construct<TargetVariant>;
        };

        struct IdentifierChainTarget
        {
            static constexpr auto rule = dsl::list(dsl::p<Common::Identifier>, dsl::sep(dsl::lit<">>">));
            static constexpr auto value =
                    Common::PmrAsList<std::pmr::vector<Ast::Common::Identifier>> >> lexy::construct<TargetVariant>;
        };

        static constexpr auto rule =
                (dsl::peek(dsl::lit_c<'"'>) >> dsl::p<StringTarget>) | (dsl::else_ >> dsl::p<IdentifierChainTarget>);
        static constexpr auto value = lexy::forward<TargetVariant>;
    };

    static constexpr auto rule = []
    {
        auto kind = dsl::p<LegalizationClauseKind>;
        auto types = dsl::parenthesized.opt_list(dsl::p<TypeConstraint>, dsl::sep(dsl::lit_c<','>));
        auto optTarget = dsl::opt(dsl::lit<">>"> >> dsl::p<TargetParser>);

        return kind + types + optTarget;
    }();

    static constexpr auto value =
            Common::PmrAsList<std::pmr::vector<Ast::LegalizeActionDef::TypeConstraint>> >>
            lexy::callback<Ast::LegalizeActionDef::LegalizeActionClause>(
                    [](Ast::LegalizeActionDef::LegalizeActionKind kind, auto constraints, auto target)
                    {
                        Ast::LegalizeActionDef::LegalizeActionClause clause;
                        clause.m_kind = kind;

                        if constexpr (std::is_same_v<std::decay_t<decltype(constraints)>,
                                                     std::pmr::vector<Ast::LegalizeActionDef::TypeConstraint>>)
                        {
                            clause.m_types = std::move(constraints);
                        }

                        if constexpr (std::is_same_v<std::decay_t<decltype(target)>, TargetVariant>)
                        {
                            std::visit(
                                    [&](auto &&val)
                                    {
                                        using T = std::decay_t<decltype(val)>;
                                        if constexpr (std::is_same_v<T, Ast::Common::StringLiteral>)
                                        {
                                            clause.m_libcallSymbol = val;
                                        }
                                        else if constexpr (std::is_same_v<T, std::pmr::vector<Ast::Common::Identifier>>)
                                        {
                                            if (clause.m_kind == Ast::LegalizeActionDef::LegalizeActionKind::Custom)
                                            {
                                                clause.m_customRules = std::move(val);
                                            }
                                            else if (!val.empty())
                                            {
                                                clause.m_targetType = std::move(val.front());
                                            }
                                        }
                                    },
                                    target);
                        }

                        return clause;
                    });
};

struct LegalizeInstructionDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"action">::rule >>
            (dsl::p<Common::Identifier> + dsl::curly_bracketed.list(dsl::p<LegalizationClause> + dsl::lit_c<';'>)) >>
            dsl::lit_c<';'>;

    static constexpr auto value =
            Common::PmrAsList<std::pmr::vector<Ast::LegalizeActionDef::LegalizeActionClause>> >>
            lexy::callback<Ast::LegalizeActionDef::LegalizeInstructionDecl>(
                    [](Ast::Common::Identifier name,
                       std::pmr::vector<Ast::LegalizeActionDef::LegalizeActionClause> actions)
                    { return Ast::LegalizeActionDef::LegalizeInstructionDecl{ std::move(name), std::move(actions) }; });
};

struct LegalizeActionFile
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::terminator(dsl::eof).list(dsl::p<LegalizeInstructionDecl>);
    static constexpr auto value =
            Common::PmrAsList<std::pmr::vector<Ast::LegalizeActionDef::LegalizeInstructionDecl>> >>
            lexy::construct<Ast::LegalizeActionDef::LegalizeActionFile>;
};

} // namespace DSL::Parser::LegalizeActionDef

#endif // EZDSL_LEGALIZE_ACTION_LANG_H