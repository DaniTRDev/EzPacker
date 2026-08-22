#ifndef EZDSL_PARSER_INST_DEF_LANG_H
#define EZDSL_PARSER_INST_DEF_LANG_H

#include "EzDslCommon.h"
#include "Ast/InstDefLangAst.h"
#include "Parser/CommonParsers.h"

namespace DSL::Parser::InstDef
{
namespace dsl = ::lexy::dsl;

struct BitSlice
{
    static constexpr auto rule =
            dsl::square_bracketed(dsl::p<Common::IntegerLiteral> + dsl::lit<':'> + dsl::p<Common::IntegerLiteral>);

    static constexpr auto value = lexy::callback<Ast::InstDef::BitSlice>(
            [](const Ast::Common::IntegerLiteral &first, const Ast::Common::IntegerLiteral &second)
            {
                auto v1 = static_cast<uint16_t>(first.m_node);
                auto v2 = static_cast<uint16_t>(second.m_node);
                return Ast::InstDef::BitSlice{ .m_low = std::min(v1, v2), .m_high = std::max(v1, v2) };
            });
};

struct SlicedIdentifier
{
    static constexpr auto rule = dsl::p<Common::Identifier> + dsl::p<BitSlice>;
    static constexpr auto value = lexy::construct<Ast::InstDef::SlicedIdentifier>;
};

struct BitExpression : lexy::expression_production
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto atom = []
    {
        auto paren = dsl::parenthesized(dsl::recurse_branch<BitExpression>);
        auto intLit = dsl::peek(dsl::ascii::digit) >> dsl::p<Common::IntegerLiteral>;
        auto identOrSlice = dsl::else_ >> (dsl::p<Common::Identifier> + dsl::opt(dsl::p<BitSlice>));
        return paren | intLit | identOrSlice;
    }();

    struct Not : dsl::prefix_op
    {
        static constexpr auto op = dsl::op<Ast::InstDef::BitExprOp::Not>(LEXY_LIT("~"));
        using operand = dsl::atom;
    };

    struct AddSub : dsl::infix_op_left
    {
        static constexpr auto op = dsl::op<Ast::InstDef::BitExprOp::Add>(LEXY_LIT("+")) /
                dsl::op<Ast::InstDef::BitExprOp::Sub>(LEXY_LIT("-"));
        using operand = Not;
    };

    struct Shift : dsl::infix_op_left
    {
        static constexpr auto op = dsl::op<Ast::InstDef::BitExprOp::Shl>(LEXY_LIT("<<")) /
                dsl::op<Ast::InstDef::BitExprOp::Shr>(LEXY_LIT(">>"));
        using operand = AddSub;
    };

    struct And : dsl::infix_op_left
    {
        static constexpr auto op = dsl::op<Ast::InstDef::BitExprOp::And>(LEXY_LIT("&"));
        using operand = Shift;
    };

    struct Xor : dsl::infix_op_left
    {
        static constexpr auto op = dsl::op<Ast::InstDef::BitExprOp::Xor>(LEXY_LIT("^"));
        using operand = And;
    };

    struct Or : dsl::infix_op_left
    {
        static constexpr auto op = dsl::op<Ast::InstDef::BitExprOp::Or>(LEXY_LIT("|"));
        using operand = Xor;
    };

    using operation = Or;

    static constexpr auto value = lexy::callback<Ast::InstDef::BitExprValues>(
            [](Ast::InstDef::BitExprValues expr) -> Ast::InstDef::BitExprValues { return expr; },
            [](Ast::Common::IntegerLiteral lit) -> Ast::InstDef::BitExprValues
            { return Ast::InstDef::BitExprValues{ std::move(lit) }; },
            [](Ast::Common::Identifier id, auto slice) -> Ast::InstDef::BitExprValues
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(slice)>, Ast::InstDef::BitSlice>)
                {
                    return Ast::InstDef::BitExprValues{ Ast::InstDef::SlicedIdentifier{ std::move(id), slice } };
                }
                else
                {
                    return Ast::InstDef::BitExprValues{ std::move(id) };
                }
            },
            [](Ast::InstDef::BitExprOp op, Ast::InstDef::BitExprValues operand) -> Ast::InstDef::BitExprValues
            {
                auto node = std::make_shared<Ast::InstDef::BitExpression>();
                node->m_lhs = std::move(operand);
                node->m_op = op;
                node->m_rhs = std::nullopt;
                return Ast::InstDef::BitExprValues{ std::move(node) };
            },
            [](Ast::InstDef::BitExprValues lhs,
               Ast::InstDef::BitExprOp op,
               Ast::InstDef::BitExprValues rhs) -> Ast::InstDef::BitExprValues
            {
                auto node = std::make_shared<Ast::InstDef::BitExpression>();
                node->m_lhs = std::move(lhs);
                node->m_op = op;
                node->m_rhs = std::move(rhs);
                return Ast::InstDef::BitExprValues{ std::move(node) };
            });
};

struct BitExprAssign
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = []
    {
        auto lhs_field = dsl::p<Common::Identifier>;
        auto opt_slice = dsl::opt(dsl::p<BitSlice>);
        auto eq_expr = dsl::lit<"="> >> dsl::p<BitExpression>;
        return lhs_field + opt_slice + eq_expr;
    }();

    static constexpr auto value = lexy::callback<Ast::InstDef::BitExprAssign>(
            [](Ast::Common::Identifier lhs, auto slice, Ast::InstDef::BitExprValues rhs)
            {
                std::optional<Ast::InstDef::BitSlice> optSlice;
                if constexpr (std::is_same_v<std::decay_t<decltype(slice)>, Ast::InstDef::BitSlice>)
                {
                    optSlice = slice;
                }
                return Ast::InstDef::BitExprAssign{ .m_lhs = std::move(lhs),
                                                    .m_lhsSlice = optSlice,
                                                    .m_rhs = std::move(rhs) };
            });
};

struct FormatField
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = []
    {
        auto name = dsl::p<Common::Identifier>;
        auto slice = dsl::p<BitSlice>;
        auto opt_default = dsl::opt(dsl::lit<"="> >> dsl::p<BitExpression>);
        auto separator = dsl::lit<";">;
        return name + slice + opt_default + separator;
    }();

    static constexpr auto value = lexy::callback<Ast::InstDef::FormatField>(
            [](Ast::Common::Identifier name, Ast::InstDef::BitSlice slice, auto defVal)
            {
                std::optional<Ast::InstDef::BitExprValues> optDefault;
                if constexpr (std::is_same_v<std::decay_t<decltype(defVal)>, Ast::InstDef::BitExprValues>)
                {
                    optDefault = std::move(defVal);
                }
                return Ast::InstDef::FormatField{ .m_name = std::move(name),
                                                  .m_slice = slice,
                                                  .m_defaultValue = std::move(optDefault) };
            });
};

struct InstFormatDecl
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = []
    {
        auto kw = Common::Keyword<"format">::rule;
        auto name = dsl::p<Common::Identifier>;
        auto opt_width = dsl::opt(dsl::parenthesized(dsl::p<Common::IntegerLiteral>));
        auto fields = dsl::curly_bracketed.list(dsl::p<FormatField>);
        return kw >> (name + opt_width + fields);
    }();

    static constexpr auto
            value = lexy::as_list<std::vector<Ast::InstDef::FormatField>> >>
            lexy::callback<Ast::InstDef::InstFormatDecl>(
                            [](Ast::Common::Identifier name, auto width, std::vector<Ast::InstDef::FormatField> fields)
                            {
                                uint32_t bitWidth = 32;
                                if constexpr (std::is_same_v<std::decay_t<decltype(width)>, Common::IntegerLiteral>)
                                {
                                    bitWidth = static_cast<uint32_t>(width.m_node);
                                }

                                Ast::InstDef::InstFormatDecl decl;
                                decl.m_name = std::move(name);
                                decl.m_bitWidth = bitWidth;
                                for (auto &f : fields)
                                {
                                    decl.m_fields.push_back(std::move(f));
                                }
                                return decl;
                            });
};

struct Direction
{
    static constexpr auto Table = lexy::symbol_table<Ast::InstDef::InstOperandDir>
        .map(LEXY_LIT("INOUT"), Ast::InstDef::InstOperandDir::ArgInOut)
        .map(LEXY_LIT("IN"), Ast::InstDef::InstOperandDir::ArgIn)
        .map(LEXY_LIT("OUT"), Ast::InstDef::InstOperandDir::ArgOut);

    static constexpr auto rule = dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha));
    static constexpr auto value = lexy::forward<Ast::InstDef::InstOperandDir>;
};

struct InstFlag
{
    static constexpr auto Table = lexy::symbol_table<Ast::InstDef::InstFlag>
        .map(LEXY_LIT("isBranch"), Ast::InstDef::InstFlag::IsBranch)
        .map(LEXY_LIT("isCall"), Ast::InstDef::InstFlag::IsCall)
        .map(LEXY_LIT("isReturn"), Ast::InstDef::InstFlag::IsReturn)
        .map(LEXY_LIT("isTerminator"), Ast::InstDef::InstFlag::IsTerminator)
        .map(LEXY_LIT("mayLoad"), Ast::InstDef::InstFlag::MayLoad)
        .map(LEXY_LIT("mayStore"), Ast::InstDef::InstFlag::MayStore)
        .map(LEXY_LIT("commutative"), Ast::InstDef::InstFlag::IsCommutative)
        .map(LEXY_LIT("volatile"), Ast::InstDef::InstFlag::HasSideEffects);

    static constexpr auto rule = dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha_underscore));
    static constexpr auto value = lexy::forward<Ast::InstDef::InstFlag>;
};

struct InstArgItem
{
    static constexpr auto whitespace = Common::Whitespace;

    struct ImmTag
    {
        static constexpr auto rule = Common::Keyword<"imm">::rule;
        static constexpr auto value = lexy::constant(true);
    };

    static constexpr auto rule = []
    {
        auto type_or_class = dsl::p<Common::Identifier>;
        auto colon = dsl::lit<':'>;
        auto optImm = dsl::opt(dsl::p<ImmTag>);
        auto name = dsl::p<Common::Identifier>;
        auto dir = dsl::p<Direction>;
        return type_or_class + colon + optImm + name + dir;
    }();

    static constexpr auto value = lexy::callback<Ast::InstDef::InstArgValues>(
            [](Ast::Common::Identifier type, auto isImm, Ast::Common::Identifier name, Ast::InstDef::InstOperandDir dir)
                    -> Ast::InstDef::InstArgValues
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(isImm)>, bool>)
                {
                    return Ast::InstDef::InstImmOperand{ .m_typeName = std::move(type),
                                                         .m_argName = std::move(name),
                                                         .m_argDir = dir };
                }
                else
                {
                    return Ast::InstDef::InstRegOperand{ .m_regClass = std::move(type),
                                                         .m_argName = std::move(name),
                                                         .m_argDir = dir };
                }
            });
};

struct InstHeaderParser
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = []
    {
        auto instKw = Common::Keyword<"inst">::rule;
        auto formatKw = Common::Keyword<"format">::rule;
        auto name = dsl::p<Common::Identifier>;
        auto args = dsl::parenthesized.list(dsl::p<InstArgItem>, dsl::sep(dsl::lit_c<','>));
        auto fmtName = dsl::p<Common::Identifier>;

        return instKw >> (name + args + formatKw + fmtName);
    }();

    static constexpr auto value = lexy::as_list<std::vector<Ast::InstDef::InstArgValues>> >>
            lexy::callback<Ast::InstDef::InstHeader>(
                                          [](Ast::Common::Identifier name,
                                             std::vector<Ast::InstDef::InstArgValues> args,
                                             Ast::Common::Identifier fmtName)
                                          {
                                              Ast::InstDef::InstHeader header;
                                              header.m_name = std::move(name);
                                              header.m_formatName = std::move(fmtName);
                                              for (auto &arg : args)
                                              {
                                                  header.m_args.push_back(std::move(arg));
                                              }
                                              return header;
                                          });
};

struct InstBodyItemParser
{
    static constexpr auto whitespace = Common::Whitespace;

    struct ImplicitDecl
    {
        static constexpr auto whitespace = Common::Whitespace;

        static constexpr auto rule = Common::Keyword<"IMPLICIT">::rule >>
                (dsl::parenthesized.list(dsl::p<InstArgItem>, dsl::sep(dsl::lit_c<','>)) + dsl::lit_c<';'>);

        static constexpr auto value = lexy::as_list<std::vector<Ast::InstDef::InstArgValues>> >>
                lexy::callback<Ast::InstDef::InstBodyItem>(
                                              [](std::vector<Ast::InstDef::InstArgValues> args)
                                              {
                                                  std::pmr::vector<Ast::InstDef::InstArgValues> pmrArgs;
                                                  for (auto &arg : args)
                                                  {
                                                      pmrArgs.push_back(std::move(arg));
                                                  }
                                                  return Ast::InstDef::InstBodyItem{ std::move(pmrArgs) };
                                              });
    };

    struct FormatDecl
    {
        static constexpr auto whitespace = Common::Whitespace;

        static constexpr auto rule = Common::Keyword<"FORMAT">::rule >>
                (dsl::parenthesized.list(dsl::p<BitExprAssign>, dsl::sep(dsl::lit_c<','>)) + dsl::lit_c<';'>);

        static constexpr auto value = lexy::as_list<std::vector<Ast::InstDef::BitExprAssign>> >>
                lexy::callback<Ast::InstDef::InstBodyItem>(
                                              [](std::vector<Ast::InstDef::BitExprAssign> assigns)
                                              {
                                                  std::pmr::vector<Ast::InstDef::BitExprAssign> pmrAssigns;
                                                  for (auto &a : assigns)
                                                  {
                                                      pmrAssigns.push_back(std::move(a));
                                                  }
                                                  return Ast::InstDef::InstBodyItem{ std::move(pmrAssigns) };
                                              });
    };

    struct FlagsDecl
    {
        static constexpr auto whitespace = Common::Whitespace;

        static constexpr auto rule = Common::Keyword<"FLAGS">::rule >>
                (dsl::parenthesized.list(dsl::p<InstFlag>, dsl::sep(dsl::lit_c<','>)) + dsl::lit_c<';'>);

        static constexpr auto value = lexy::as_list<std::vector<Ast::InstDef::InstFlag>> >>
                lexy::callback<Ast::InstDef::InstBodyItem>(
                                              [](std::vector<Ast::InstDef::InstFlag> flags)
                                              {
                                                  std::pmr::vector<Ast::InstDef::InstFlag> pmrFlags;
                                                  for (auto f : flags)
                                                  {
                                                      pmrFlags.push_back(f);
                                                  }
                                                  return Ast::InstDef::InstBodyItem{ std::move(pmrFlags) };
                                              });
    };

    struct AsmDecl
    {
        static constexpr auto whitespace = Common::Whitespace;

        static constexpr auto rule = Common::Keyword<"ASM">::rule >>
                (dsl::parenthesized(dsl::p<Common::StringLiteral>) + dsl::lit_c<';'>);

        static constexpr auto value = lexy::callback<Ast::InstDef::InstBodyItem>(
                [](Ast::Common::StringLiteral lit) { return Ast::InstDef::InstBodyItem{ std::move(lit) }; });
    };

    struct LatencyDecl
    {
        static constexpr auto whitespace = Common::Whitespace;

        static constexpr auto rule = Common::Keyword<"LATENCY">::rule >>
                (dsl::parenthesized(dsl::p<Common::IntegerLiteral>) + dsl::lit_c<';'>);

        static constexpr auto value = lexy::callback<Ast::InstDef::InstBodyItem>(
                [](Ast::Common::IntegerLiteral lit)
                { return Ast::InstDef::InstBodyItem{ static_cast<uint32_t>(lit.m_node) }; });
    };

    static constexpr auto rule = (dsl::peek(Common::Keyword<"IMPLICIT">::rule) >> dsl::p<ImplicitDecl>) |
            (dsl::peek(Common::Keyword<"FORMAT">::rule) >> dsl::p<FormatDecl>) |
            (dsl::peek(Common::Keyword<"FLAGS">::rule) >> dsl::p<FlagsDecl>) |
            (dsl::peek(Common::Keyword<"ASM">::rule) >> dsl::p<AsmDecl>) |
            (dsl::peek(Common::Keyword<"LATENCY">::rule) >> dsl::p<LatencyDecl>);

    static constexpr auto value = lexy::forward<Ast::InstDef::InstBodyItem>;
};

struct InstDeclParser
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = dsl::p<InstHeaderParser> + dsl::curly_bracketed.list(dsl::p<InstBodyItemParser>);

    static constexpr auto value =
            lexy::as_list<std::vector<Ast::InstDef::InstBodyItem>> >>
            lexy::callback<Ast::InstDef::InstDecl>(
                    [](Ast::InstDef::InstHeader header, std::vector<Ast::InstDef::InstBodyItem> items)
                    {
                        Ast::InstDef::InstDecl inst;
                        inst.m_header = std::move(header);

                        for (auto &item : items)
                        {
                            std::visit(
                                    [&](auto &&val)
                                    {
                                        using T = std::decay_t<decltype(val)>;
                                        if constexpr (std::is_same_v<T, std::pmr::vector<Ast::InstDef::InstArgValues>>)
                                        {
                                            inst.m_body.m_implicitArgs = std::move(val);
                                        }
                                        else if constexpr (std::is_same_v<
                                                                   T,
                                                                   std::pmr::vector<Ast::InstDef::BitExprAssign>>)
                                        {
                                            inst.m_body.m_assigns = std::move(val);
                                        }
                                        else if constexpr (std::is_same_v<T, Ast::Common::StringLiteral>)
                                        {
                                            inst.m_body.m_asmTemplate =
                                                    std::pmr::string(val.m_node.data(), val.m_node.size());
                                        }
                                        else if constexpr (std::is_same_v<T, uint64_t> || std::is_same_v<T, uint32_t>)
                                        {
                                            inst.m_body.m_latency = static_cast<uint32_t>(val);
                                        }
                                        else if constexpr (std::is_same_v<T, std::pmr::vector<Ast::InstDef::InstFlag>>)
                                        {
                                            inst.m_body.m_flags = std::move(val);
                                        }
                                    },
                                    item);
                        }
                        return inst;
                    });
};

using InstDecl = InstDeclParser;

struct InstDefFileParser
{
    static constexpr auto whitespace = Common::Whitespace;

    struct Entry
    {
        std::variant<Ast::InstDef::InstFormatDecl, Ast::InstDef::InstDecl> decl;
    };

    struct EntryParser
    {
        static constexpr auto whitespace = Common::Whitespace;

        static constexpr auto rule = []
        {
            auto fmtBranch = dsl::peek(Common::Keyword<"format">::rule) >> dsl::p<InstFormatDecl>;
            auto instBranch = dsl::peek(Common::Keyword<"inst">::rule) >> dsl::p<InstDeclParser>;
            return (fmtBranch | instBranch) + dsl::opt(dsl::lit_c<';'>);
        }();

        // Use auto... to consume the optional nullopt produced by dsl::opt(dsl::lit_c<';'>)
        static constexpr auto value =
                lexy::callback<Entry>([](Ast::InstDef::InstFormatDecl fmt, auto...) { return Entry{ std::move(fmt) }; },
                                      [](Ast::InstDef::InstDecl inst, auto...) { return Entry{ std::move(inst) }; });
    };

    // Use terminator.list instead of opt_list
    static constexpr auto rule = dsl::terminator(dsl::eof).list(dsl::p<EntryParser>);

    static constexpr auto value =
            lexy::as_list<std::vector<Entry>> >>
            lexy::callback<Ast::InstDef::InstDefFile>(
                    [](std::vector<Entry> entries)
                    {
                        Ast::InstDef::InstDefFile file;
                        for (auto &entry : entries)
                        {
                            if (std::holds_alternative<Ast::InstDef::InstFormatDecl>(entry.decl))
                            {
                                file.m_formats.push_back(std::get<Ast::InstDef::InstFormatDecl>(std::move(entry.decl)));
                            }
                            else
                            {
                                file.m_instructions.push_back(std::get<Ast::InstDef::InstDecl>(std::move(entry.decl)));
                            }
                        }
                        return file;
                    });
};

} // namespace DSL::Parser::InstDef

#endif // EZDSL_PARSER_INST_DEF_LANG_H