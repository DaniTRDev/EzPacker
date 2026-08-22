#ifndef EZDSL_TARGET_DEF_LANG_H
#define EZDSL_TARGET_DEF_LANG_H

#include "EzDslCommon.h"
#include "ParseContext.h"
#include "Ast/TargetDefLangAst.h"
#include "CommonParsers.h"

namespace DSL::Parser::TargetDef
{
namespace dsl = ::lexy::dsl;

/**
 * rax(, 64, 0)
 * eax(rax, 32, 0)
 */
struct TargetRegister
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = []
    {
        auto name = dsl::p<Common::Identifier>;
        // Wrap the peek branch in dsl::opt so omitting the parent identifier is valid
        auto parent = dsl::opt(dsl::peek(dsl::ascii::alpha_underscore) >> dsl::p<Common::Identifier>);
        auto size = dsl::p<Common::IntegerLiteral>;
        auto offset = dsl::p<Common::IntegerLiteral>;
        auto comma = dsl::lit_c<','>;

        return name + dsl::parenthesized(parent + comma + size + comma + offset);
    }();

    static constexpr auto value = lexy::callback<Ast::TargetDef::TargetRegister>(
            [](Ast::Common::Identifier name,
               auto parent,
               Ast::Common::IntegerLiteral size,
               Ast::Common::IntegerLiteral offset)
            {
                Ast::Common::Identifier resolvedParent;
                if constexpr (std::is_same_v<std::decay_t<decltype(parent)>, Ast::Common::Identifier>)
                {
                    resolvedParent = std::move(parent);
                }

                return Ast::TargetDef::TargetRegister{ std::move(name),
                                                       std::move(resolvedParent),
                                                       std::move(size),
                                                       std::move(offset) };
            });
};

/**
 * CLASS(gpr64,
 *     rax(, 64, 0),
 *     rcx(, 64, 0)
 * );
 */
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

    static constexpr auto value =
            lexy::as_list<std::pmr::vector<Ast::TargetDef::TargetRegister>> >>
            lexy::callback<Ast::TargetDef::TargetRegisterClass>(
                    [](Ast::Common::Identifier name, auto... rest)
                    {
                        Ast::TargetDef::TargetRegisterClass cls;
                        cls.m_name = std::move(name);

                        (
                                [&](auto &&arg)
                                {
                                    using T = std::decay_t<decltype(arg)>;
                                    if constexpr (std::is_same_v<T, std::pmr::vector<Ast::TargetDef::TargetRegister>>)
                                    {
                                        cls.m_registers = std::move(arg);
                                    }
                                }(rest),
                                ...);

                        return cls;
                    });
};

/**
 * bank MyBankName {
 *     CLASS(gpr64, ...);
 *     CLASS(fpr64, ...);
 * };
 */
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

    static constexpr auto
            value = lexy::as_list<std::pmr::vector<Ast::TargetDef::TargetRegisterClass>> >>
            lexy::callback<Ast::TargetDef::TargetRegisterBank>(
                            [](Ast::Common::Identifier name,
                               std::pmr::vector<Ast::TargetDef::TargetRegisterClass> classes,
                               auto...)
                            { return Ast::TargetDef::TargetRegisterBank{ std::move(name), std::move(classes) }; });
};

/**
 * include idef "file.idf";
 * include isel "file.isf";
 */
struct TargetIncFile
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = Common::Keyword<"include">::rule >>
            (dsl::p<Common::Identifier> + dsl::p<Common::StringLiteral> + dsl::lit_c<';'>);

    static constexpr auto value = lexy::construct<Ast::TargetDef::TargetIncFile>;
};

/**
 * target x86_64 {
 *     include idef "common.idf";
 *     bank GPR { ... };
 * };
 */
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
            value = lexy::as_list<std::vector<Item>> >>
            lexy::callback<Ast::TargetDef::TargetDef>(
                            [](Ast::Common::Identifier name, std::vector<Item> items, auto...)
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