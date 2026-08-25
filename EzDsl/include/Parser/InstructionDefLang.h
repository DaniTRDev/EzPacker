#ifndef EZDSL_PARSER_INST_DEF_LANG_H
#define EZDSL_PARSER_INST_DEF_LANG_H

#include "Ast/CommonAstNodes.h"
#include "Ast/InstructionDefLangAst.h"
#include "EzDslCommon.h"
#include "Parser/CommonParsers.h"

namespace DSL::Parser::InstDef
{
namespace dsl = ::lexy::dsl;

// Matches "[31:0]" or "[0:15]" (normalizes to min/max)
struct BitSlice
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule =
            dsl::square_bracketed(dsl::p<Common::IntegerLiteral> + dsl::lit_c<':'> + dsl::p<Common::IntegerLiteral>);

    static constexpr auto value = lexy::callback<Ast::InstDef::BitSlice>(
            [](const Ast::Common::IntegerLiteral &first, const Ast::Common::IntegerLiteral &second)
            {
                auto v1 = static_cast<uint16_t>(first.m_node);
                auto v2 = static_cast<uint16_t>(second.m_node);
                return Ast::InstDef::BitSlice{ .m_from = std::min(v1, v2), .m_to = std::max(v1, v2) };
            });
};

// Matches "ident[0:4]"
struct SlicedIdentifier
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<Common::Identifier> + dsl::p<BitSlice>;

    static constexpr auto value = lexy::callback<Ast::InstDef::SlicedIdentifier>(
            [](Ast::Common::Identifier name, Ast::InstDef::BitSlice slice)
            { return Ast::InstDef::SlicedIdentifier{ .m_name = std::move(name), .m_slice = slice }; });
};

struct BitExpression : lexy::expression_production
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto atom = []
    {
        auto paren = dsl::parenthesized(dsl::recurse_branch<BitExpression>);
        auto intLit =
                dsl::peek(dsl::ascii::digit | dsl::lit_c<'-'> | dsl::lit_c<'+'>) >> dsl::p<Common::IntegerLiteral>;
        auto slicedId = dsl::peek(dsl::p<Common::Identifier> + dsl::lit_c<'['>) >> dsl::p<SlicedIdentifier>;
        auto bareId = dsl::else_ >> dsl::p<Common::Identifier>;
        return paren | intLit | slicedId | bareId;
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

    static constexpr auto value = lexy::bind(
            lexy::callback<Ast::InstDef::BitExprValues>(
                    [](ParseContext &ctx, Ast::InstDef::BitExprValues expr) -> Ast::InstDef::BitExprValues
                    { return expr; },
                    [](ParseContext &ctx, Ast::Common::IntegerLiteral lit) -> Ast::InstDef::BitExprValues
                    { return Ast::InstDef::BitExprValues{ lit }; },
                    [](ParseContext &ctx, Ast::InstDef::SlicedIdentifier sId) -> Ast::InstDef::BitExprValues
                    { return Ast::InstDef::BitExprValues{ std::move(sId) }; },
                    [](ParseContext &ctx, Ast::Common::Identifier id) -> Ast::InstDef::BitExprValues
                    { return Ast::InstDef::BitExprValues{ std::move(id) }; },
                    [](ParseContext &ctx, Ast::InstDef::BitExprOp op, Ast::InstDef::BitExprValues operand)
                            -> Ast::InstDef::BitExprValues
                    {
                        auto *alloc = ctx.getAllocator();
                        void *mem = alloc->allocate(sizeof(Ast::InstDef::BitExpression),
                                                    alignof(Ast::InstDef::BitExpression));
                        auto *node = new (mem) Ast::InstDef::BitExpression{ std::move(operand), op, std::nullopt };
                        return Ast::InstDef::BitExprValues{ node };
                    },
                    [](ParseContext &ctx,
                       Ast::InstDef::BitExprValues lhs,
                       Ast::InstDef::BitExprOp op,
                       Ast::InstDef::BitExprValues rhs) -> Ast::InstDef::BitExprValues
                    {
                        auto *alloc = ctx.getAllocator();
                        void *mem = alloc->allocate(sizeof(Ast::InstDef::BitExpression),
                                                    alignof(Ast::InstDef::BitExpression));
                        auto *node = new (mem) Ast::InstDef::BitExpression{ std::move(lhs), op, std::move(rhs) };
                        return Ast::InstDef::BitExprValues{ node };
                    }),
            lexy::parse_state,
            lexy::values);
};

// Matches "opcode = 0x33" or "imm4_0[0:4] = imm12[0:4]"
struct BitExprAssign
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = []
    {
        auto lhs_field = dsl::p<Common::Identifier>;
        auto opt_slice = dsl::opt(dsl::peek(dsl::lit_c<'['>) >> dsl::p<BitSlice>);
        auto eq_expr = dsl::lit_c<'='> >> dsl::p<BitExpression>;
        return lhs_field + opt_slice + eq_expr;
    }();

    static constexpr auto value = lexy::callback<Ast::InstDef::BitExprAssign>(
            [](Ast::Common::Identifier lhs, Ast::InstDef::BitSlice slice, Ast::InstDef::BitExprValues rhs) {
                return Ast::InstDef::BitExprAssign{ .m_lhs = std::move(lhs),
                                                    .m_lhsSlice = slice,
                                                    .m_rhs = std::move(rhs) };
            },
            [](Ast::Common::Identifier lhs, lexy::nullopt, Ast::InstDef::BitExprValues rhs)
            {
                return Ast::InstDef::BitExprAssign{ .m_lhs = std::move(lhs),
                                                    .m_lhsSlice = std::nullopt,
                                                    .m_rhs = std::move(rhs) };
            });
};

// Matches "opcode[0:6];" or "funct3[12:14] = 0b000;"
struct FormatField
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = []
    {
        auto name = dsl::p<Common::Identifier>;
        auto slice = dsl::p<BitSlice>;
        auto opt_default = dsl::opt(dsl::lit_c<'='> >> dsl::p<BitExpression>);
        auto separator = dsl::lit_c<';'>;
        return name + slice + opt_default + separator;
    }();

    static constexpr auto value = lexy::callback<Ast::InstDef::FormatField>(
            [](Ast::Common::Identifier name, Ast::InstDef::BitSlice slice, Ast::InstDef::BitExprValues defVal)
            {
                return Ast::InstDef::FormatField{ .m_name = std::move(name),
                                                  .m_slice = slice,
                                                  .m_defaultValue = std::move(defVal) };
            },
            [](Ast::Common::Identifier name, Ast::InstDef::BitSlice slice, lexy::nullopt) {
                return Ast::InstDef::FormatField{ .m_name = std::move(name),
                                                  .m_slice = slice,
                                                  .m_defaultValue = std::nullopt };
            });
};

// Matches "format RType(32) { ... };"
struct InstFormatDecl
{
    static constexpr auto whitespace = Common::Whitespace;

    struct FieldList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<FormatField>);
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::InstDef::FormatField>>;
    };

    static constexpr auto rule = Common::Keyword<"format">::rule >>
            (dsl::p<Common::Identifier> + dsl::opt(dsl::parenthesized(dsl::p<Common::IntegerLiteral>)) +
             dsl::p<FieldList> + dsl::opt(dsl::lit_c<';'>));

    static constexpr auto value = lexy::callback<Ast::InstDef::InstFormatDecl>(
            [](Ast::Common::Identifier name,
               Ast::Common::IntegerLiteral width,
               std::pmr::vector<Ast::InstDef::FormatField> fields,
               auto...)
            {
                return Ast::InstDef::InstFormatDecl{ .m_name = std::move(name),
                                                     .m_bitWidth = static_cast<uint32_t>(width.m_node),
                                                     .m_fields = std::move(fields) };
            },
            [](Ast::Common::Identifier name, lexy::nullopt, std::pmr::vector<Ast::InstDef::FormatField> fields, auto...)
            {
                return Ast::InstDef::InstFormatDecl{ .m_name = std::move(name),
                                                     .m_bitWidth = 32u,
                                                     .m_fields = std::move(fields) };
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

// Matches "GPR:rd OUT", "simm(i12):imm12 IN", "imm:val IN"
struct InstOperand
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = []
    {
        auto typeParam = dsl::parenthesized(dsl::p<Common::Identifier>);
        auto type_or_class = dsl::p<Common::Identifier> + dsl::opt(typeParam);
        auto colon = dsl::lit_c<':'>;
        auto name = dsl::p<Common::Identifier>;
        auto dir = dsl::p<Direction>;
        return type_or_class + colon + name + dir;
    }();

    static constexpr auto value = lexy::callback<Ast::InstDef::InstOperand>(
            [](Ast::Common::Identifier type,
               Ast::Common::Identifier param,
               Ast::Common::Identifier name,
               Ast::InstDef::InstOperandDir dir)
            {
                return Ast::InstDef::InstOperand{ .m_kind = Ast::InstDef::InstOperandKind::Immediate,
                                                  .m_typeOrClass = std::move(type),
                                                  .m_typeParam = std::move(param),
                                                  .m_name = std::move(name),
                                                  .m_dir = dir };
            },
            [](Ast::Common::Identifier type,
               lexy::nullopt,
               Ast::Common::Identifier name,
               Ast::InstDef::InstOperandDir dir)
            {
                bool isImm = (type.m_node == "imm" || type.m_node == "simm" || type.m_node == "uimm");
                auto kind = isImm ? Ast::InstDef::InstOperandKind::Immediate : Ast::InstDef::InstOperandKind::Register;

                return Ast::InstDef::InstOperand{ .m_kind = kind,
                                                  .m_typeOrClass = std::move(type),
                                                  .m_typeParam = std::nullopt,
                                                  .m_name = std::move(name),
                                                  .m_dir = dir };
            });
};

// Matches "inst SW(GPR:rs2 IN, GPR:rs1 IN, simm(i12):imm12 IN) format SType"
struct InstHeader
{
    static constexpr auto whitespace = Common::Whitespace;

    struct InstArgList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::list(dsl::p<InstOperand>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<Ast::InstDef::InstOperand>;
    };

    static constexpr auto rule = Common::Keyword<"inst">::rule >>
            (dsl::p<Common::Identifier> +
             dsl::parenthesized(dsl::opt(dsl::peek(dsl::ascii::alpha_digit_underscore) >> dsl::p<InstArgList>)) +
             Common::Keyword<"format">::rule + dsl::p<Common::Identifier>);

    static constexpr auto value = lexy::callback<Ast::InstDef::InstHeader>(
            [](Ast::Common::Identifier name, auto args, Ast::Common::Identifier fmtName)
            {
                std::pmr::vector<Ast::InstDef::InstOperand> resolvedArgs;
                if constexpr (std::is_same_v<std::decay_t<decltype(args)>, std::pmr::vector<Ast::InstDef::InstOperand>>)
                {
                    resolvedArgs = std::move(args);
                }

                return Ast::InstDef::InstHeader{ .m_name = std::move(name),
                                                 .m_args = std::move(resolvedArgs),
                                                 .m_formatName = std::move(fmtName) };
            });
};

struct InstBodyItem
{
    static constexpr auto whitespace = Common::Whitespace;

    struct ImplicitDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"IMPLICIT">::rule >>
                (dsl::parenthesized.list(dsl::p<InstOperand>, dsl::sep(dsl::lit_c<','>)) + dsl::lit_c<';'>);
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::InstDef::InstOperand>>;
    };

    struct FormatDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"FORMAT">::rule >>
                (dsl::parenthesized.list(dsl::p<BitExprAssign>, dsl::sep(dsl::lit_c<','>)) + dsl::lit_c<';'>);
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::InstDef::BitExprAssign>>;
    };

    struct FlagsDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"FLAGS">::rule >>
                (dsl::parenthesized.list(dsl::p<InstFlag>, dsl::sep(dsl::lit_c<','>)) + dsl::lit_c<';'>);
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::InstDef::InstFlag>>;
    };

    struct AsmDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"ASM">::rule >>
                (dsl::parenthesized(dsl::p<Common::StringLiteral>) + dsl::lit_c<';'>);
        static constexpr auto value = lexy::forward<Ast::Common::StringLiteral>;
    };

    struct LatencyDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"LATENCY">::rule >>
                (dsl::parenthesized(dsl::p<Common::IntegerLiteral>) + dsl::lit_c<';'>);
        static constexpr auto value = lexy::callback<uint32_t>([](Ast::Common::IntegerLiteral lit)
                                                               { return static_cast<uint32_t>(lit.m_node); });
    };

    static constexpr auto rule = (dsl::peek(Common::Keyword<"IMPLICIT">::rule) >> dsl::p<ImplicitDecl>) |
            (dsl::peek(Common::Keyword<"FORMAT">::rule) >> dsl::p<FormatDecl>) |
            (dsl::peek(Common::Keyword<"FLAGS">::rule) >> dsl::p<FlagsDecl>) |
            (dsl::peek(Common::Keyword<"ASM">::rule) >> dsl::p<AsmDecl>) |
            (dsl::peek(Common::Keyword<"LATENCY">::rule) >> dsl::p<LatencyDecl>);

    static constexpr auto value = lexy::construct<Ast::InstDef::InstBodyItem>;
};

struct InstDecl
{
    static constexpr auto whitespace = Common::Whitespace;

    struct InstBodyList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<InstBodyItem>);
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::InstDef::InstBodyItem>>;
    };

    static constexpr auto rule = dsl::p<InstHeader> + dsl::p<InstBodyList> + dsl::opt(dsl::lit_c<';'>);

    static constexpr auto value = lexy::callback<Ast::InstDef::InstDecl>(
            [](Ast::InstDef::InstHeader header, std::pmr::vector<Ast::InstDef::InstBodyItem> items, auto...)
            {
                Ast::InstDef::InstDecl inst;
                inst.m_header = std::move(header);

                for (auto &item : items)
                {
                    std::visit(
                            [&](auto &&val)
                            {
                                using T = std::decay_t<decltype(val)>;
                                if constexpr (std::is_same_v<T, std::pmr::vector<Ast::InstDef::InstOperand>>)
                                {
                                    inst.m_body.m_implicitArgs = std::move(val);
                                }
                                else if constexpr (std::is_same_v<T, std::pmr::vector<Ast::InstDef::BitExprAssign>>)
                                {
                                    inst.m_body.m_assigns = std::move(val);
                                }
                                else if constexpr (std::is_same_v<T, Ast::Common::StringLiteral>)
                                {
                                    inst.m_body.m_asmTemplate = std::pmr::string(val.m_node.data(), val.m_node.size());
                                }
                                else if constexpr (std::is_same_v<T, uint32_t>)
                                {
                                    inst.m_body.m_latency = val;
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

struct InstDefFile
{
    static constexpr auto whitespace = Common::Whitespace;

    struct EntryItem
    {
        std::variant<Ast::InstDef::InstFormatDecl, Ast::InstDef::InstDecl> decl;
    };

    struct Entry
    {
        static constexpr auto whitespace = Common::Whitespace;

        static constexpr auto rule = []
        {
            auto fmtBranch = dsl::peek(Common::Keyword<"format">::rule) >> dsl::p<InstFormatDecl>;
            auto instBranch = dsl::peek(Common::Keyword<"inst">::rule) >> dsl::p<InstDecl>;
            return (fmtBranch | instBranch);
        }();

        static constexpr auto value =
                lexy::callback<EntryItem>([](Ast::InstDef::InstFormatDecl fmt) { return EntryItem{ std::move(fmt) }; },
                                          [](Ast::InstDef::InstDecl inst) { return EntryItem{ std::move(inst) }; });
    };

    static constexpr auto rule = dsl::terminator(dsl::eof).list(dsl::p<Entry>);

    static constexpr auto value =
            Common::PmrAsList<std::pmr::vector<EntryItem>> >>
            lexy::callback<Ast::InstDef::InstDefFile>(
                    [](std::pmr::vector<EntryItem> entries)
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