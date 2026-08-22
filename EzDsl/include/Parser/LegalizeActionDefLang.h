#ifndef EZDSL_LEGALIZE_ACTION_LANG_H
#define EZDSL_LEGALIZE_ACTION_LANG_H

#include "Ast/LegalizeActionDefLangAst.h"
#include "EzDslCommon.h"
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
                Ast::LegalizeActionDef::TypeConstraint typeConstraint;
                typeConstraint.m_type = std::move(typeName);

                if constexpr (!std::is_same_v<std::decay_t<decltype(index)>, lexy::nullopt>)
                {
                    typeConstraint.m_typeIndex = std::move(index);
                }

                return typeConstraint;
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

    static constexpr auto rule = []()
    {
        auto kind = dsl::p<LegalizationClauseKind>;
        auto types = dsl::list(dsl::p<TypeConstraint>, dsl::sep(dsl::lit_c<','>));
        auto op = dsl::lit<">>">;
        auto target =
                dsl::peek(dsl::lit_c<'"'>) >> dsl::p<Common::StringLiteral> | dsl::else_ >> dsl::p<Common::Identifier>;

        return kind + dsl::parenthesized(types) + op + target;
    }();

    static constexpr auto value = lexy::as_list<std::pmr::vector<Ast::LegalizeActionDef::TypeConstraint>> >>
            lexy::callback<Ast::LegalizeActionDef::LegalizeActionClause>(
                                          [](Ast::LegalizeActionDef::LegalizeActionKind kind,
                                             std::pmr::vector<Ast::LegalizeActionDef::TypeConstraint> constraints,
                                             Ast::Common::StringLiteral libcall)
                                          {
                                              Ast::LegalizeActionDef::LegalizeActionClause clause;
                                              clause.m_kind = kind;
                                              clause.m_types = std::move(constraints);
                                              clause.m_libcallSymbol = std::move(libcall);
                                              return clause;
                                          },
                                          [](Ast::LegalizeActionDef::LegalizeActionKind kind,
                                             std::pmr::vector<Ast::LegalizeActionDef::TypeConstraint> constraints,
                                             Ast::Common::Identifier targetType)
                                          {
                                              Ast::LegalizeActionDef::LegalizeActionClause clause;
                                              clause.m_kind = kind;
                                              clause.m_types = std::move(constraints);
                                              clause.m_targetType = std::move(targetType);
                                              return clause;
                                          });
};

struct InstructionLegalizeDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"action">::rule >> dsl::p<Common::Identifier> +
                    dsl::curly_bracketed.list(dsl::p<LegalizationClause> + dsl::lit_c<';'>) + dsl::lit_c<';'>;

    static constexpr auto value = lexy::as_list<std::pmr::vector<Ast::LegalizeActionDef::LegalizeActionClause>> >>
            lexy::callback<Ast::LegalizeActionDef::InstructionLegalizeDecl>(
                                          [](Ast::Common::Identifier name,
                                             std::pmr::vector<Ast::LegalizeActionDef::LegalizeActionClause> actions)
                                          {
                                              Ast::LegalizeActionDef::InstructionLegalizeDecl decl;
                                              decl.m_instName = std::move(name);
                                              decl.m_actions = std::move(actions);
                                              return decl;
                                          });
};

struct TargetLegalizeDef
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::terminator(dsl::eof).list(dsl::p<InstructionLegalizeDecl>);
    static constexpr auto value = lexy::as_list<std::pmr::vector<Ast::LegalizeActionDef::InstructionLegalizeDecl>> >>
            lexy::callback<Ast::LegalizeActionDef::TargetLegalizeDef>(
                                          [](std::pmr::vector<Ast::LegalizeActionDef::InstructionLegalizeDecl> actions)
                                          {
                                              Ast::LegalizeActionDef::TargetLegalizeDef target;
                                              target.m_instructionActions = std::move(actions);
                                              return target;
                                          });
};
} // namespace DSL::Parser::LegalizeActionDef

#endif // EZDSL_LEGALIZE_ACTION_LANG_H