#ifndef EZDSL_REGISTER_DEF_LANG_H
#define EZDSL_REGISTER_DEF_LANG_H

#include "Ast/CommonAstNodes.h"
#include "Ast/RegisterDefLangAst.h"
#include "EzDslLexerCommon.h"
#include "Parser/CommonParsers.h"

namespace DSL::Parser::RegisterDef
{
namespace dsl = ::lexy::dsl;

using Ast::RegisterDef::RegisterBankDecl;
using Ast::RegisterDef::RegisterClassDecl;
using Ast::RegisterDef::RegisterDecl;
using Ast::RegisterDef::RegisterFile;
using Ast::RegisterDef::RegisterNameBinding;
using Ast::RegisterDef::SpecialRegDecl;
using Ast::RegisterDef::SubRegisterEdge;

/**
 * Parses a single `NAME: BITS` register class entry.
 */
struct RegisterClassEntry
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<Common::Identifier> + dsl::lit_c<':'> + dsl::p<Common::IntegerLiteral>;
    static constexpr auto value = lexy::callback<RegisterClassDecl>(
            [](Ast::Common::Identifier name, Ast::Common::IntegerLiteral bits)
            { return RegisterClassDecl{ .m_name = std::move(name), .m_bitSize = std::move(bits) }; });
};

/**
 * Parses a `classes { ... }` block into a PMR list of RegisterClassDecl.
 */
struct ClassesBlock
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"classes">::rule >>
            dsl::curly_bracketed.list(dsl::p<RegisterClassEntry>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = Common::PmrAsList<RegisterClassDecl>;
};

/**
 * Parses a `WIDE <: NARROW` sub-register relation.
 */
struct SubRegisterEntry
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<Common::Identifier> + LEXY_LIT("<:") + dsl::p<Common::Identifier>;
    static constexpr auto value = lexy::callback<SubRegisterEdge>(
            [](Ast::Common::Identifier wide, Ast::Common::Identifier narrow)
            { return SubRegisterEdge{ .m_wideClass = std::move(wide), .m_narrowClass = std::move(narrow) }; });
};

/**
 * Parses a `sub_register { ... }` block into a PMR list of SubRegisterEdge relations.
 */
struct SubRegisterBlock
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"sub_register">::rule >>
            dsl::curly_bracketed.list(dsl::p<SubRegisterEntry>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = Common::PmrAsList<SubRegisterEdge>;
};

/**
 * Parses a `NAME: CLASS` printable-name binding inside a register `names { ... }` block.
 */
struct RegisterNameEntry
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<Common::Identifier> + dsl::lit_c<':'> + dsl::p<Common::Identifier>;
    static constexpr auto value = lexy::callback<RegisterNameBinding>(
            [](Ast::Common::Identifier asmName, Ast::Common::Identifier className)
            { return RegisterNameBinding{ .m_asmName = std::move(asmName), .m_className = std::move(className) }; });
};

/**
 * Parses a `names { ... }` block into a PMR list of per-class assembly-name bindings.
 */
struct RegisterNameBlock
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"names">::rule >>
            dsl::curly_bracketed.list(dsl::p<RegisterNameEntry>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = Common::PmrAsList<RegisterNameBinding>;
};

/**
 * Parses one physical register: `NAME enc N names { asm: CLASS, ... }`.
 */
struct RegisterEntry
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule =
            dsl::p<Common::Identifier> + LEXY_LIT("enc") + dsl::p<Common::IntegerLiteral> + dsl::p<RegisterNameBlock>;
    static constexpr auto value = lexy::callback<RegisterDecl>(
            [](Ast::Common::Identifier name,
               Ast::Common::IntegerLiteral enc,
               std::pmr::vector<RegisterNameBinding> names)
            {
                return RegisterDecl{ .m_canonicalName = std::move(name),
                                     .m_encoding = std::move(enc),
                                     .m_names = std::move(names) };
            });
};

/**
 * Parses a `registers { ... }` block into a PMR list of physical RegisterDecls.
 */
struct RegistersBlock
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"registers">::rule >> dsl::curly_bracketed.list(dsl::p<RegisterEntry>);
    static constexpr auto value = Common::PmrAsList<RegisterDecl>;
};

/**
 * Parses a whole register bank:
 *   register_bank NAME { classes { ... } sub_register { ... } registers { ... } }
 * The `sub_register` block is optional.
 */
struct RegisterBankParser
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"register_bank">::rule >>
            (dsl::p<Common::Identifier> +
             dsl::curly_bracketed(dsl::p<ClassesBlock> +
                                  dsl::opt(dsl::peek(Common::Keyword<"sub_register">::rule) >>
                                           dsl::p<SubRegisterBlock>) +
                                  dsl::p<RegistersBlock>));
    static constexpr auto value = lexy::callback<RegisterBankDecl>(
            [](Ast::Common::Identifier name,
               std::pmr::vector<RegisterClassDecl> classes,
               std::optional<std::pmr::vector<SubRegisterEdge>> edges,
               std::pmr::vector<RegisterDecl> registers)
            {
                RegisterBankDecl decl{};
                decl.m_name = std::move(name);
                decl.m_classes = std::move(classes);
                if (edges.has_value())
                {
                    decl.m_subRegisterEdges = std::move(*edges);
                }
                decl.m_registers = std::move(registers);
                return decl;
            });
};

/**
 * Parses one `NAME: ID` entry inside the `special { ... }` block.
 */
struct SpecialRegEntry
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<Common::Identifier> + dsl::lit_c<':'> + dsl::p<Common::IntegerLiteral>;
    static constexpr auto value = lexy::callback<SpecialRegDecl>(
            [](Ast::Common::Identifier name, Ast::Common::IntegerLiteral id)
            { return SpecialRegDecl{ .m_name = std::move(name), .m_id = std::move(id) }; });
};

/**
 * Parses a `special { ... }` block into a PMR list of pseudo-register declarations.
 */
struct SpecialBlock
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"special">::rule >>
            dsl::curly_bracketed.list(dsl::p<SpecialRegEntry>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = Common::PmrAsList<SpecialRegDecl>;
};

/**
 * Top-level item variant: a register bank or the special-register block.
 */
using TopLevelItem = std::variant<RegisterBankDecl, std::pmr::vector<SpecialRegDecl>>;

/**
 * Wraps a parsed register bank as a top-level item.
 */
struct RegisterBankTopLevel
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<RegisterBankParser>;
    static constexpr auto value =
            lexy::callback<TopLevelItem>([](RegisterBankDecl bank) { return TopLevelItem{ std::move(bank) }; });
};

/**
 * Wraps a parsed special-register block as a top-level item.
 */
struct SpecialTopLevel
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<SpecialBlock>;
    static constexpr auto value = lexy::callback<TopLevelItem>([](std::pmr::vector<SpecialRegDecl> special)
                                                               { return TopLevelItem{ std::move(special) }; });
};

/**
 * Parses the sequence of register banks and special blocks at file scope.
 */
struct TopLevelList
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule =
            dsl::list((dsl::peek(Common::Keyword<"register_bank">::rule) >> dsl::p<RegisterBankTopLevel>) |
                      (dsl::peek(Common::Keyword<"special">::rule) >> dsl::p<SpecialTopLevel>));
    static constexpr auto value = Common::PmrAsList<TopLevelItem>;
};

/**
 * Root rule for a `.reg` file.
 */
struct RegisterDefFile
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"target">::rule >>
            ((dsl::p<Common::Identifier> + dsl::lit_c<';'>)+dsl::p<TopLevelList>);
    static constexpr auto value = lexy::callback<RegisterFile>(
            [](Ast::Common::Identifier target, std::pmr::vector<TopLevelItem> items)
            {
                RegisterFile file{};
                file.m_target = std::move(target);
                for (auto &item : items)
                {
                    if (auto *bank = std::get_if<RegisterBankDecl>(&item))
                    {
                        file.m_banks.push_back(std::move(*bank));
                    }
                    else if (auto *special = std::get_if<std::pmr::vector<SpecialRegDecl>>(&item))
                    {
                        for (auto &reg : *special)
                        {
                            file.m_specialRegs.push_back(std::move(reg));
                        }
                    }
                }
                return file;
            });
};

} // namespace DSL::Parser::RegisterDef

#endif // EZDSL_REGISTER_DEF_LANG_H
