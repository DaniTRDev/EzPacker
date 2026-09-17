#ifndef EZDSL_PARSER_TARGET_INST_DEF_LANG_H
#define EZDSL_PARSER_TARGET_INST_DEF_LANG_H

#include "Ast/CommonAstNodes.h"
#include "Ast/TargetInstDefLangAst.h"
#include "EzDslLexerCommon.h"
#include "Parser/CommonParsers.h"

namespace DSL::Parser::TargetInstDef
{
namespace dsl = ::lexy::dsl;

struct Direction
{
    static constexpr auto Table =
        lexy::symbol_table<Ast::TargetInstDef::OperandDirection>
            .map(LEXY_LIT("IN"), Ast::TargetInstDef::OperandDirection::In)
            .map(LEXY_LIT("OUT"), Ast::TargetInstDef::OperandDirection::Out)
            .map(LEXY_LIT("INOUT"), Ast::TargetInstDef::OperandDirection::InOut);

    static constexpr auto rule = dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha));
    static constexpr auto value = lexy::forward<Ast::TargetInstDef::OperandDirection>;
};

struct TargetOperand
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<Common::Identifier> + dsl::lit_c<':'> + dsl::p<Common::Identifier> + dsl::p<Direction>;

    static constexpr auto value = lexy::callback<Ast::TargetInstDef::TargetOperandDecl>(
        [](Ast::Common::Identifier regClassOrType, Ast::Common::Identifier name, Ast::TargetInstDef::OperandDirection dir)
        {
            return Ast::TargetInstDef::TargetOperandDecl{
                .m_regClassOrType = std::move(regClassOrType),
                .m_name = std::move(name),
                .m_direction = dir
            };
        });
};

struct BodyItem
{
    static constexpr auto whitespace = Common::Whitespace;

    struct TagMnemonic { Ast::Common::StringLiteral val; };
    struct TagFlags { std::pmr::vector<Ast::Common::Identifier> val; };
    struct TagImplicitDefs { std::pmr::vector<Ast::Common::Identifier> val; };
    struct TagImplicitUses { std::pmr::vector<Ast::Common::Identifier> val; };

    using ItemVariant = std::variant<TagMnemonic, TagFlags, TagImplicitDefs, TagImplicitUses>;

    struct IdList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::list(dsl::p<Common::Identifier>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::Common::Identifier>>;
    };

    struct OptIdList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::parenthesized(
            dsl::opt(dsl::peek(dsl::ascii::alpha_digit_underscore) >> dsl::p<IdList>));
        static constexpr auto value = lexy::callback<std::pmr::vector<Ast::Common::Identifier>>(
            [](std::pmr::vector<Ast::Common::Identifier> list) { return list; },
            [](lexy::nullopt) { return std::pmr::vector<Ast::Common::Identifier>{}; });
    };

    struct MnemonicDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"MNEMONIC">::rule >>
            (dsl::parenthesized(dsl::p<Common::StringLiteral>) + dsl::lit_c<';'>);
        static constexpr auto value = lexy::callback<TagMnemonic>(
            [](Ast::Common::StringLiteral s) { return TagMnemonic{ std::move(s) }; });
    };

    struct FlagsDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"FLAGS">::rule >>
            (dsl::p<OptIdList> + dsl::lit_c<';'>);
        static constexpr auto value = lexy::callback<TagFlags>(
            [](std::pmr::vector<Ast::Common::Identifier> list) { return TagFlags{ std::move(list) }; });
    };

    struct ImplicitDefsDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"IMPLICIT_DEFS">::rule >>
            (dsl::p<OptIdList> + dsl::lit_c<';'>);
        static constexpr auto value = lexy::callback<TagImplicitDefs>(
            [](std::pmr::vector<Ast::Common::Identifier> list) { return TagImplicitDefs{ std::move(list) }; });
    };

    struct ImplicitUsesDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"IMPLICIT_USES">::rule >>
            (dsl::p<OptIdList> + dsl::lit_c<';'>);
        static constexpr auto value = lexy::callback<TagImplicitUses>(
            [](std::pmr::vector<Ast::Common::Identifier> list) { return TagImplicitUses{ std::move(list) }; });
    };

    static constexpr auto rule = (dsl::peek(Common::Keyword<"MNEMONIC">::rule) >> dsl::p<MnemonicDecl>) |
                                 (dsl::peek(Common::Keyword<"FLAGS">::rule) >> dsl::p<FlagsDecl>) |
                                 (dsl::peek(Common::Keyword<"IMPLICIT_DEFS">::rule) >> dsl::p<ImplicitDefsDecl>) |
                                 (dsl::peek(Common::Keyword<"IMPLICIT_USES">::rule) >> dsl::p<ImplicitUsesDecl>);

    static constexpr auto value = lexy::forward<ItemVariant>;
};

struct TargetInstDecl
{
    static constexpr auto whitespace = Common::Whitespace;

    struct NonEmptyOperandList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::list(dsl::p<TargetOperand>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::TargetInstDef::TargetOperandDecl>>;
    };

    struct OperandList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::parenthesized(
            dsl::opt(dsl::peek(dsl::ascii::alpha_digit_underscore) >> dsl::p<NonEmptyOperandList>));
        static constexpr auto value = lexy::callback<std::pmr::vector<Ast::TargetInstDef::TargetOperandDecl>>(
            [](std::pmr::vector<Ast::TargetInstDef::TargetOperandDecl> list) { return list; },
            [](lexy::nullopt) { return std::pmr::vector<Ast::TargetInstDef::TargetOperandDecl>{}; });
    };

    struct BodyList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<BodyItem>);
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<BodyItem::ItemVariant>>;
    };

    static constexpr auto rule = Common::Keyword<"target_inst">::rule >>
        (dsl::p<Common::Identifier> + dsl::p<OperandList> + dsl::p<BodyList> + dsl::opt(dsl::lit_c<';'>));

    static constexpr auto value = lexy::callback<Ast::TargetInstDef::TargetInstDecl>(
        [](Ast::Common::Identifier name,
           std::pmr::vector<Ast::TargetInstDef::TargetOperandDecl> operands,
           std::pmr::vector<BodyItem::ItemVariant> bodyItems,
           auto...)
        {
            Ast::TargetInstDef::TargetInstDecl decl;
            decl.m_instName = std::move(name);
            decl.m_operands = std::move(operands);

            for (auto &item : bodyItems)
            {
                std::visit(
                    [&](auto &&val)
                    {
                        using T = std::decay_t<decltype(val)>;
                        if constexpr (std::is_same_v<T, BodyItem::TagMnemonic>)
                            decl.m_mnemonic = std::move(val.val);
                        else if constexpr (std::is_same_v<T, BodyItem::TagFlags>)
                            decl.m_flags = std::move(val.val);
                        else if constexpr (std::is_same_v<T, BodyItem::TagImplicitDefs>)
                            decl.m_implicitDefs = std::move(val.val);
                        else if constexpr (std::is_same_v<T, BodyItem::TagImplicitUses>)
                            decl.m_implicitUses = std::move(val.val);
                    },
                    item);
            }
            return decl;
        });
};

struct TargetHeader
{
    static constexpr auto rule = Common::Keyword<"target">::rule >> (dsl::p<Common::Identifier> + dsl::lit_c<';'>);
    static constexpr auto value = lexy::forward<Ast::Common::Identifier>;
};

struct InstItemParser
{
    static constexpr auto rule = dsl::peek(Common::Keyword<"target_inst">::rule) >> dsl::p<TargetInstDecl>;
    static constexpr auto value = lexy::forward<Ast::TargetInstDef::TargetInstDecl>;
};

struct TargetInstFile
{
    static constexpr auto whitespace = Common::Whitespace;

    struct InstList
    {
        static constexpr auto rule = dsl::list(dsl::p<InstItemParser>);
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::TargetInstDef::TargetInstDecl>>;
    };

    static constexpr auto rule = dsl::terminator(dsl::eof)(dsl::opt(dsl::p<TargetHeader>) + dsl::p<InstList>);

    static constexpr auto value = lexy::callback<Ast::TargetInstDef::TargetInstFile>(
        [](auto targetOpt, std::pmr::vector<Ast::TargetInstDef::TargetInstDecl> insts)
        {
            Ast::TargetInstDef::TargetInstFile file;
            if constexpr (std::is_same_v<std::decay_t<decltype(targetOpt)>, Ast::Common::Identifier>)
            {
                file.m_targetName = std::move(targetOpt);
            }
            file.m_instructions = std::move(insts);
            return file;
        });
};

} // namespace DSL::Parser::TargetInstDef

#endif // EZDSL_PARSER_TARGET_INST_DEF_LANG_H
