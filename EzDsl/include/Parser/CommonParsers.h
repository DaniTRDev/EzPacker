#ifndef EZDSL_COMMON_PARSERS_H
#define EZDSL_COMMON_PARSERS_H

#include "Ast/CommonAstNodes.h"
#include "EzDslCommon.h"
#include "ParseContext.h"

#include <charconv>

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

struct Comment
{
    static constexpr auto rule = dsl::lit_c<'/'> >> dsl::lit_c<'/'> >> dsl::until(dsl::newline);
};

static constexpr auto Whitespace = dsl::ascii::space | dsl::inline_<Comment> | dsl::ascii::newline;

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

struct IntegerLiteral
{
    static constexpr auto rule = []
    {
        auto sign = dsl::opt(dsl::p<SingleChar<'-'>>);

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

    static constexpr auto value =
            lexy::bind(lexy::callback<Ast::Common::RealLiteral>(
                               [](ParseContext &ctx, const char *startIter, auto textLexeme, const char *endIter)
                               {
                                   std::string_view text(textLexeme.data(), textLexeme.size());
                                   double parsedValue = 0.0;
                                   std::from_chars(text.data(), text.data() + text.size(), parsedValue);
                                   return Ast::Common::RealLiteral{ parsedValue, ctx.createRef(startIter, endIter) };
                               }),
                       lexy::parse_state,
                       lexy::values);
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

template <typename T> struct PmrContainerTraits
{
    using container_type = std::pmr::vector<T>;
    using value_type = T;
};

template <typename T, typename Alloc> struct PmrContainerTraits<std::vector<T, Alloc>>
{
    using container_type = std::vector<T, Alloc>;
    using value_type = T;
};

template <typename Target> struct PmrListSink
{
    using traits = PmrContainerTraits<Target>;
    using return_type = typename traits::container_type;

    struct _sink
    {
        using return_type = typename traits::container_type;
        return_type _cont;

        explicit _sink(std::pmr::memory_resource *mr) : _cont(mr ? mr : std::pmr::get_default_resource()) {}

        template <typename U> void operator()(U &&item) { _cont.push_back(std::forward<U>(item)); }

        return_type finish() && { return std::move(_cont); }
    };

    template <typename State> _sink sink(State &state) const
    {
        if constexpr (requires { state.memoryResource(); })
        {
            return _sink(state.memoryResource());
        }
        else
        {
            return _sink(std::pmr::get_default_resource());
        }
    }

    _sink sink() const { return _sink(std::pmr::get_default_resource()); }
};

// Used to automatically make std::pmr lists allocate using the allocator of the ParseContext structure.
template <typename ValueType> constexpr PmrListSink<ValueType> PmrAsList{};

} // namespace DSL::Parser::Common

#endif // EZDSL_COMMON_PARSERS_H