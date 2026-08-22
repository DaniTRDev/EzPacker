#ifndef EZDSL_TARGET_DEF_LANG_H
#define EZDSL_TARGET_DEF_LANG_H

#include "Ast/TargetDefLangAst.h"
#include "EzDslCommon.h"
#include "ParseContext.h"
#include "Parser/CommonParsers.h"

namespace DSL::Parser::TargetDef
{
namespace dsl = ::lexy::dsl;

struct TargetRegister
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = []
    {
        auto name = dsl::p<Common::Identifier>;
        auto parent = dsl::opt(dsl::peek(dsl::ascii::alpha_underscore) >> dsl::p<Common::Identifier>);
        auto size = dsl::p<Common::IntegerLiteral>;
        auto offset = dsl::p<Common::IntegerLiteral>;
        auto comma = dsl::lit_c<','>;

        return name + dsl::parenthesized(parent + comma + size + comma + offset);
    }();

    static constexpr auto value = lexy::callback<Ast::TargetDef::TargetRegister>(
            [](Ast::Common::Identifier name,
               Ast::Common::Identifier parent,
               Ast::Common::IntegerLiteral size,
               Ast::Common::IntegerLiteral offset)
            { return Ast::TargetDef::TargetRegister{ std::move(name), std::move(parent), size, offset }; },
            [](Ast::Common::Identifier name,
               lexy::nullopt,
               Ast::Common::IntegerLiteral size,
               Ast::Common::IntegerLiteral offset)
            { return Ast::TargetDef::TargetRegister{ std::move(name), Ast::Common::Identifier{}, size, offset }; });
};

struct TargetRegisterClass
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = []
    {
        auto kw = Common::Keyword<"CLASS">::rule;
        auto name = dsl::p<Common::Identifier>;
        auto regList = dsl::opt(dsl::lit_c<','> >> dsl::list(dsl::p<TargetRegister>, dsl::sep(dsl::lit_c<','>)));

        return kw >> (dsl::parenthesized(name + regList) + dsl::lit_c<';'>);
    }();

    static constexpr auto
            value = Common::PmrAsList<std::pmr::vector<Ast::TargetDef::TargetRegister>> >>
            lexy::callback<Ast::TargetDef::TargetRegisterClass>(
                            [](Ast::Common::Identifier name, std::pmr::vector<Ast::TargetDef::TargetRegister> registers)
                            { return Ast::TargetDef::TargetRegisterClass{ std::move(name), std::move(registers) }; },
                            [](Ast::Common::Identifier name, lexy::nullopt)
                            { return Ast::TargetDef::TargetRegisterClass{ std::move(name), {} }; });
};

struct TargetRegisterBank
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = []
    {
        auto kw = Common::Keyword<"bank">::rule;
        auto name = dsl::p<Common::Identifier>;
        auto classes = dsl::curly_bracketed.list(dsl::p<TargetRegisterClass>);

        return kw >> (name + classes + dsl::opt(dsl::lit_c<';'>));
    }();

    static constexpr auto value =
            Common::PmrAsList<std::pmr::vector<Ast::TargetDef::TargetRegisterClass>> >>
            lexy::callback<Ast::TargetDef::TargetRegisterBank>(
                    [](Ast::Common::Identifier name, std::pmr::vector<Ast::TargetDef::TargetRegisterClass> classes)
                    { return Ast::TargetDef::TargetRegisterBank{ std::move(name), std::move(classes) }; },
                    [](Ast::Common::Identifier name,
                       std::pmr::vector<Ast::TargetDef::TargetRegisterClass> classes,
                       lexy::nullopt)
                    { return Ast::TargetDef::TargetRegisterBank{ std::move(name), std::move(classes) }; });
};

struct TargetIncFileType
{
    static constexpr auto TypeTable = lexy::symbol_table<Ast::TargetDef::TargetIncludeFileType>
        .map(LEXY_LIT("idf"), Ast::TargetDef::TargetIncludeFileType::InstructionDef)
        .map(LEXY_LIT("lad"), Ast::TargetDef::TargetIncludeFileType::LegalizeActionDef)
        .map(LEXY_LIT("lrd"), Ast::TargetDef::TargetIncludeFileType::LegalizeRuleDef)
        .map(LEXY_LIT("isf"), Ast::TargetDef::TargetIncludeFileType::InstructionSelDef);

    static constexpr auto rule = dsl::symbol<TypeTable>(dsl::identifier(dsl::ascii::alpha));
    static constexpr auto value = lexy::forward<Ast::TargetDef::TargetIncludeFileType>;
};

struct TargetIncFile
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = Common::Keyword<"include">::rule >> dsl::p<TargetIncFileType> >>
            (dsl::p<Common::StringLiteral> + dsl::lit_c<';'>);

    static constexpr auto value = lexy::construct<Ast::TargetDef::TargetIncFile>;
};

struct TargetDef
{
    static constexpr auto whitespace = Common::Whitespace;

    struct Item
    {
        std::variant<Ast::TargetDef::TargetIncFile, Ast::TargetDef::TargetRegisterBank> value;
    };

    struct ItemParser
    {
        static constexpr auto whitespace = Common::Whitespace;

        static constexpr auto rule = []
        {
            auto bankBranch = dsl::peek(Common::Keyword<"bank">::rule) >> dsl::p<TargetRegisterBank>;
            auto incBranch = dsl::peek(Common::Keyword<"include">::rule) >> dsl::p<TargetIncFile>;
            return bankBranch | incBranch;
        }();

        static constexpr auto value =
                lexy::callback<Item>([](Ast::TargetDef::TargetRegisterBank bank) { return Item{ std::move(bank) }; },
                                     [](Ast::TargetDef::TargetIncFile inc) { return Item{ std::move(inc) }; });
    };

    static constexpr auto rule = []
    {
        auto kw = Common::Keyword<"target">::rule;
        auto name = dsl::p<Common::Identifier>;
        auto body = dsl::curly_bracketed.list(dsl::p<ItemParser>);

        return kw >> (name + body + dsl::opt(dsl::lit_c<';'>)) + dsl::eof;
    }();

    static constexpr auto
            value = Common::PmrAsList<std::pmr::vector<Item>> >>
            lexy::callback<Ast::TargetDef::TargetDef>(
                            [](Ast::Common::Identifier name, std::pmr::vector<Item> items)
                            {
                                Ast::TargetDef::TargetDef target;
                                target.m_name = std::move(name);

                                for (auto &item : items)
                                {
                                    if (std::holds_alternative<Ast::TargetDef::TargetIncFile>(item.value))
                                    {
                                        target.m_inclusions.push_back(
                                                std::get<Ast::TargetDef::TargetIncFile>(std::move(item.value)));
                                    }
                                    else
                                    {
                                        target.m_regBanks.push_back(
                                                std::get<Ast::TargetDef::TargetRegisterBank>(std::move(item.value)));
                                    }
                                }
                                return target;
                            },
                            [](Ast::Common::Identifier name, std::pmr::vector<Item> items, lexy::nullopt)
                            {
                                Ast::TargetDef::TargetDef target;
                                target.m_name = std::move(name);

                                for (auto &item : items)
                                {
                                    if (std::holds_alternative<Ast::TargetDef::TargetIncFile>(item.value))
                                    {
                                        target.m_inclusions.push_back(
                                                std::get<Ast::TargetDef::TargetIncFile>(std::move(item.value)));
                                    }
                                    else
                                    {
                                        target.m_regBanks.push_back(
                                                std::get<Ast::TargetDef::TargetRegisterBank>(std::move(item.value)));
                                    }
                                }
                                return target;
                            });
};

} // namespace DSL::Parser::TargetDef

#endif // EZDSL_TARGET_DEF_LANG_H