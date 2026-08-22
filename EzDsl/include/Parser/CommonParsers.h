#ifndef EZDSL_COMMON_PARSERS_H
#define EZDSL_COMMON_PARSERS_H

#include "EzDslCommon.h"
#include "ParseContext.h"
#include "Ast/CommonAstNodes.h"

namespace DSL::Parser::Common
{
namespace dsl = ::lexy::dsl;

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

template <char C> struct SingleChar
{
    static constexpr auto rule = dsl::lit_c<C>;
    static constexpr auto value = lexy::constant(true);
};

struct IntegerLiteral
{
    static constexpr auto rule = []
    {
        // Optional Sign (-)
        auto sign = dsl::opt(dsl::p<SingleChar<'-'>>);

        // Base prefixes and digit parsers
        auto hex = (dsl::lit<"0x"> | dsl::lit<"0X">) >> dsl::integer<uint64_t, dsl::hex>(dsl::digits<dsl::hex>);
        auto bin = (dsl::lit<"0b"> | dsl::lit<"0B">) >> dsl::integer<uint64_t, dsl::binary>(dsl::digits<dsl::binary>);
        auto oct = (dsl::lit<"0o"> | dsl::lit<"0O">) >> dsl::integer<uint64_t, dsl::octal>(dsl::digits<dsl::octal>);
        auto dec = dsl::integer<uint64_t>(dsl::digits<dsl::decimal>);

        auto number = hex | bin | oct | dec;

        return dsl::position + sign + number + dsl::position;
    }();

    static constexpr auto value = lexy::bind(
            lexy::callback<Ast::Common::IntegerLiteral>(
                    [](ParseContext &ctx, const char *startIter, auto isNegative, uint64_t val, const char *endIter)
                    {
                        int64_t signedVal = 0;

                        if constexpr (!std::is_same_v<std::decay_t<decltype(isNegative)>, lexy::nullopt>)
                        {
                            signedVal = -static_cast<int64_t>(val);
                        }
                        else
                        {
                            signedVal = static_cast<int64_t>(val);
                        }

                        SourceReference *ref = ctx.createRef(startIter, endIter);
                        return Ast::Common::IntegerLiteral{ signedVal, ref };
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