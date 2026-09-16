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
        .map(LEXY_LIT("LOWER"), Ast::LegalizeActionDef::LegalizeActionKind::Lower)
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
        auto types = dsl::opt(dsl::parenthesized.opt_list(dsl::p<TypeConstraint>, dsl::sep(dsl::lit_c<','>)));
        auto optTarget = dsl::opt(dsl::lit<">>"> >> dsl::p<TargetParser>);

        return kind + types + optTarget;
    }();

    static constexpr auto value =
            Common::PmrAsList<std::pmr::vector<Ast::LegalizeActionDef::TypeConstraint>> >>
            lexy::callback<Ast::LegalizeActionDef::LegalizeActionClause>(
                    [](Ast::LegalizeActionDef::LegalizeActionKind kind,
                       auto constraints,
                       auto target)
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
                                            else if (clause.m_kind == Ast::LegalizeActionDef::LegalizeActionKind::Lower)
                                            {
                                                if (!val.empty())
                                                {
                                                    clause.m_lowerHandler = std::move(val.front());
                                                }
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

struct ClampScalarClause
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"CLAMP_SCALAR">::rule >>
            dsl::parenthesized(dsl::p<Common::Identifier> + dsl::lit_c<','> + dsl::p<Common::Identifier>);

    static constexpr auto value = lexy::callback<Ast::LegalizeActionDef::ClampScalarClause>(
            [](Ast::Common::Identifier minType, Ast::Common::Identifier maxType)
            {
                return Ast::LegalizeActionDef::ClampScalarClause{ std::move(minType), std::move(maxType) };
            });
};

using ActionItem = std::variant<Ast::LegalizeActionDef::LegalizeActionClause, Ast::LegalizeActionDef::ClampScalarClause>;

struct ActionItemParser
{
    struct ClampItem
    {
        static constexpr auto rule = dsl::p<ClampScalarClause>;
        static constexpr auto value = lexy::construct<ActionItem>;
    };

    struct ClauseItem
    {
        static constexpr auto rule = dsl::p<LegalizationClause>;
        static constexpr auto value = lexy::construct<ActionItem>;
    };

    static constexpr auto rule =
            (dsl::peek(Common::Keyword<"CLAMP_SCALAR">::rule) >> dsl::p<ClampItem>) |
            (dsl::else_ >> dsl::p<ClauseItem>);

    static constexpr auto value = lexy::forward<ActionItem>;
};

struct ActionItemList
{
    static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<ActionItemParser> + dsl::lit_c<';'>);
    static constexpr auto value = Common::PmrAsList<std::pmr::vector<ActionItem>>;
};

struct TypeSetDecl
{
    static constexpr auto whitespace = Common::Whitespace;

    struct TypeList
    {
        static constexpr auto rule =
                dsl::parenthesized.list(dsl::p<Common::Identifier>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::Common::Identifier>>;
    };

    static constexpr auto rule = Common::Keyword<"type_set">::rule >>
            (dsl::p<Common::Identifier> + dsl::lit_c<'='> + dsl::p<TypeList>) >>
            dsl::lit_c<';'>;

    static constexpr auto value = lexy::callback<Ast::LegalizeActionDef::TypeSetDecl>(
            [](Ast::Common::Identifier name, std::pmr::vector<Ast::Common::Identifier> types)
            {
                return Ast::LegalizeActionDef::TypeSetDecl{ std::move(name), std::move(types) };
            });
};

struct InstructionGroupDecl
{
    static constexpr auto whitespace = Common::Whitespace;

    struct InstructionList
    {
        static constexpr auto rule =
                dsl::parenthesized.list(dsl::p<Common::Identifier>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::Common::Identifier>>;
    };

    static constexpr auto rule = Common::Keyword<"group">::rule >>
            (dsl::p<Common::Identifier> + dsl::lit_c<'='> + dsl::p<InstructionList> + dsl::p<ActionItemList>) >>
            dsl::lit_c<';'>;

    static constexpr auto value = lexy::callback<Ast::LegalizeActionDef::InstructionGroupDecl>(
            [](Ast::Common::Identifier groupName,
               std::pmr::vector<Ast::Common::Identifier> instructions,
               std::pmr::vector<ActionItem> items)
            {
                Ast::LegalizeActionDef::InstructionGroupDecl groupDecl;
                groupDecl.m_groupName = std::move(groupName);
                groupDecl.m_instructions = std::move(instructions);
                for (auto &item : items)
                {
                    if (std::holds_alternative<Ast::LegalizeActionDef::ClampScalarClause>(item))
                    {
                        groupDecl.m_clampClause = std::move(std::get<Ast::LegalizeActionDef::ClampScalarClause>(item));
                    }
                    else if (std::holds_alternative<Ast::LegalizeActionDef::LegalizeActionClause>(item))
                    {
                        groupDecl.m_actionClauses.push_back(
                                std::move(std::get<Ast::LegalizeActionDef::LegalizeActionClause>(item)));
                    }
                }
                return groupDecl;
            });
};

struct LegalizeInstructionDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"action">::rule >>
            (dsl::p<Common::Identifier> + dsl::p<ActionItemList>) >>
            dsl::lit_c<';'>;

    static constexpr auto value = lexy::callback<Ast::LegalizeActionDef::LegalizeInstructionDecl>(
            [](Ast::Common::Identifier name, std::pmr::vector<ActionItem> items)
            {
                Ast::LegalizeActionDef::LegalizeInstructionDecl decl;
                decl.m_instName = std::move(name);
                for (auto &item : items)
                {
                    if (std::holds_alternative<Ast::LegalizeActionDef::ClampScalarClause>(item))
                    {
                        decl.m_clampClause = std::move(std::get<Ast::LegalizeActionDef::ClampScalarClause>(item));
                    }
                    else if (std::holds_alternative<Ast::LegalizeActionDef::LegalizeActionClause>(item))
                    {
                        decl.m_actionClauses.push_back(
                                std::move(std::get<Ast::LegalizeActionDef::LegalizeActionClause>(item)));
                    }
                }
                return decl;
            });
};

struct TargetDecl
{
    static constexpr auto rule = Common::Keyword<"target">::rule >> (dsl::p<Common::Identifier> + dsl::lit_c<';'>);
    static constexpr auto value = lexy::forward<Ast::Common::Identifier>;
};

using FileItem = std::variant<Ast::LegalizeActionDef::TypeSetDecl,
                              Ast::LegalizeActionDef::InstructionGroupDecl,
                              Ast::LegalizeActionDef::LegalizeInstructionDecl>;

struct FileItemParser
{
    struct TypeSetItem
    {
        static constexpr auto rule = dsl::p<TypeSetDecl>;
        static constexpr auto value = lexy::construct<FileItem>;
    };

    struct GroupItem
    {
        static constexpr auto rule = dsl::p<InstructionGroupDecl>;
        static constexpr auto value = lexy::construct<FileItem>;
    };

    struct ActionItem
    {
        static constexpr auto rule = dsl::p<LegalizeInstructionDecl>;
        static constexpr auto value = lexy::construct<FileItem>;
    };

    static constexpr auto rule =
            (dsl::peek(Common::Keyword<"type_set">::rule) >> dsl::p<TypeSetItem>) |
            (dsl::peek(Common::Keyword<"group">::rule) >> dsl::p<GroupItem>) |
            (dsl::peek(Common::Keyword<"action">::rule) >> dsl::p<ActionItem>);

    static constexpr auto value = lexy::forward<FileItem>;
};

struct LegalizeActionFile
{
    static constexpr auto whitespace = Common::Whitespace;

    struct FileItemList
    {
        static constexpr auto rule = dsl::list(dsl::p<FileItemParser>);
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<FileItem>>;
    };

    static constexpr auto rule = dsl::terminator(dsl::eof)(dsl::opt(dsl::p<TargetDecl>) + dsl::p<FileItemList>);

    static constexpr auto value = lexy::callback<Ast::LegalizeActionDef::LegalizeActionFile>(
            [](auto targetOpt, std::pmr::vector<FileItem> items)
            {
                Ast::LegalizeActionDef::LegalizeActionFile file;
                if constexpr (std::is_same_v<std::decay_t<decltype(targetOpt)>, Ast::Common::Identifier>)
                {
                    file.m_targetName = std::move(targetOpt);
                }
                for (auto &item : items)
                {
                    std::visit(
                            [&](auto &&val)
                            {
                                using T = std::decay_t<decltype(val)>;
                                if constexpr (std::is_same_v<T, Ast::LegalizeActionDef::TypeSetDecl>)
                                {
                                    file.m_typeSets.push_back(std::move(val));
                                }
                                else if constexpr (std::is_same_v<T, Ast::LegalizeActionDef::InstructionGroupDecl>)
                                {
                                    file.m_groups.push_back(std::move(val));
                                }
                                else if constexpr (std::is_same_v<T, Ast::LegalizeActionDef::LegalizeInstructionDecl>)
                                {
                                    file.m_legalizeInstrDecls.push_back(std::move(val));
                                }
                            },
                            item);
                }
                return file;
            });
};

} // namespace DSL::Parser::LegalizeActionDef

#endif // EZDSL_LEGALIZE_ACTION_LANG_H