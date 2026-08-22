#ifndef EZDSL_COMMON_PARSERS_H
#define EZDSL_COMMON_PARSERS_H

#include "EzDslCommon.h"
#include "ParseContext.h"
#include "Ast/CommonAstNodes.h"

namespace DSL::Parser::Common
{
namespace dsl = ::lexy::dsl;

struct IntegerLiteral
{
    static constexpr auto rule = []
    {
        auto hex = (dsl::lit<"0x"> | dsl::lit<"0X">) >> dsl::integer<uint64_t, dsl::hex>(dsl::digits<dsl::hex>);
        auto dec = dsl::integer<uint64_t>(dsl::digits<dsl::decimal>);
        return dsl::position + (hex | dec) + dsl::position;
    }();

    static constexpr auto value =
            lexy::bind(lexy::callback<Ast::Common::IntegerLiteral>(
                               [](ParseContext &ctx, const char *startIter, int64_t val, const char *endIter)
                               {
                                   SourceReference *ref = ctx.createRef(startIter, endIter);
                                   return Ast::Common::IntegerLiteral{ std::move(val), ref };
                               }),
                       lexy::parse_state,
                       lexy::values);
};

struct RealLiteral
{
    static constexpr auto FloatRule =
            dsl::token(dsl::digits<> >> (dsl::period + dsl::digits<>) | dsl::period >> dsl::digits<>);

    static constexpr auto rule = dsl::position + dsl::capture(FloatRule) + dsl::position;

    static constexpr auto value = lexy::bind(
            lexy::callback<Ast::Common::RealLiteral>(
                    [](ParseContext &ctx, const char *startIter, auto textLexeme, const char *endIter)
                    {
                        std::string_view text(textLexeme.data(), textLexeme.size());
                        double parsedValue = 0.0;
                        std::from_chars(text.data(), text.data() + text.size(), parsedValue);
                        return Ast::Common::RealLiteral{ { parsedValue, ctx.createRef(startIter, endIter) } };
                    }),
            lexy::parse_state,
            lexy::values);
};

struct Identifier
{
    static constexpr auto rule = dsl::position +
            dsl::identifier(dsl::ascii::alpha_underscore, dsl::ascii::alpha_digit_underscore) + dsl::position;

    static constexpr auto value =
            lexy::bind(lexy::callback<Ast::Common::Identifier>(
                               [](ParseContext &ctx, const char *startIter, auto lexeme, const char *endIter)
                               {
                                   SourceReference *ref = ctx.createRef(startIter, endIter);
                                   std::string_view str(lexeme.data(), lexeme.size());
                                   return Ast::Common::Identifier{ str, ref };
                               }),
                       lexy::parse_state,
                       lexy::values);
};

struct Comment
{
    static constexpr auto rule = dsl::lit_c<'/'> >> dsl::lit_c<'/'> >> dsl::until(dsl::newline);
};

template <lexy::_detail::string_literal KeywordStr> struct Keyword
{
    static constexpr auto rule = []
    {
        auto head = dsl::ascii::alpha_underscore;
        auto tail = dsl::ascii::alpha_digit_underscore;
        auto id = dsl::identifier(head, tail);
        return LEXY_KEYWORD(KeywordStr, id);
    }();

    static constexpr auto value = lexy::constant(true);
};

struct StringLiteral
{
    static constexpr auto rule = []
    {
        auto content =
                dsl::capture(dsl::token(dsl::while_(dsl::ascii::character - dsl::ascii::control - dsl::lit_c<'"'>)));

        return dsl::position + dsl::lit_c<'"'> + content + dsl::lit_c<'"'> + dsl::position;
    }();

    static constexpr auto value =
            lexy::bind(lexy::callback<Ast::Common::StringLiteral>(
                               [](ParseContext &ctx, const char *startIter, auto lexeme, const char *endIter)
                               {
                                   SourceReference *ref = ctx.createRef(startIter, endIter);
                                   std::string_view str(lexeme.data(), lexeme.size());
                                   return Ast::Common::StringLiteral{ str, ref };
                               }),
                       lexy::parse_state,
                       lexy::values);
};

static constexpr auto Whitespace = dsl::ascii::space | dsl::inline_<Common::Comment> | dsl::ascii::newline;

}; // namespace DSL::Parser::Common

#endif // EZDSL_COMMON_PARSERS_H