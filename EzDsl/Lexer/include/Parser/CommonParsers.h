#ifndef EZDSL_COMMON_PARSERS_H
#define EZDSL_COMMON_PARSERS_H

#include "Ast/CommonAstNodes.h"
#include "EzDslLexerCommon.h"
#include "ParseContext.h"

namespace DSL::Parser::Common
{
namespace dsl = ::lexy::dsl;

/**
 * Matches a reserved keyword as a whole identifier (word-boundary aware) and yields `true`.
 */
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

/**
 * Matches a single literal character and yields `true`.
 */
template <char C> struct SingleChar
{
    static constexpr auto rule = dsl::lit_c<C>;
    static constexpr auto value = lexy::constant(true);
};

/**
 * Matches a `// ...` line comment up to (but not including) the newline.
 */
struct Comment
{
    static constexpr auto rule = dsl::lit_c<'/'> >> dsl::lit_c<'/'> >> dsl::until(dsl::newline);
};

/**
 * Lexy symbol table matching boolean literal tokens ('true', 'false', case-insensitive).
 */
struct BooleanLit
{
    static constexpr auto Table = lexy::symbol_table<bool>
        .map(LEXY_LIT("true"), true)
        .map(LEXY_LIT("TRUE"), true)
        .map(LEXY_LIT("false"), false)
        .map(LEXY_LIT("FALSE"), false);

    static constexpr auto rule = dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha));
    static constexpr auto value = lexy::forward<bool>;
};

/**
 * Parses a boolean literal with surrounding positions and produces a located Common::BooleanLiteral.
 */
struct BooleanLiteral
{
    static constexpr auto rule = dsl::position + dsl::p<BooleanLit> + dsl::position;

    static constexpr auto value =
            lexy::bind(lexy::callback<Ast::Common::BooleanLiteral>(
                               [](ParseContext &ctx, const char *startIter, bool val, const char *endIter)
                               {
                                   SourceReference *ref = ctx.createRef(startIter, endIter);
                                   return Ast::Common::BooleanLiteral{ val, ref };
                               }),
                       lexy::parse_state,
                       lexy::values);
};

static constexpr auto Whitespace = dsl::ascii::space | dsl::inline_<Comment> | dsl::ascii::newline;

/**
 * Parses an identifier with surrounding positions and produces a located Common::Identifier
 * holding a view into the source buffer.
 */
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

/**
 * Parses a signed integer literal in decimal, hex (0x), binary (0b), or octal (0o) form and
 * produces a located Common::IntegerLiteral with sign applied.
 */
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

/**
 * Parses a double-quoted string literal and produces a located Common::StringLiteral holding a
 * view of its contents.
 */
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

/**
 * Maps an element type to the container type a list rule should produce; defaults to
 * std::pmr::vector so parsed lists land in the arena.
 */
template <typename T> struct PmrContainerTraits
{
    using container_type = std::pmr::vector<T>;
    using value_type = T;
};

/**
 * Specialization that preserves an explicitly supplied standard-allocator vector type.
 */
template <typename T, typename Alloc> struct PmrContainerTraits<std::vector<T, Alloc>>
{
    using container_type = std::vector<T, Alloc>;
    using value_type = T;
};

/**
 * Custom Lexy list sink allocating std::pmr::vector instances using the ParseContext arena memory resource.
 */
template <typename Target> struct PmrListSink
{
    using traits = PmrContainerTraits<Target>;
    using return_type = typename traits::container_type;

    constexpr return_type operator()(return_type &&container) const { return std::move(container); }

    constexpr return_type operator()(lexy::nullopt) const { return return_type(std::pmr::get_default_resource()); }

    template <typename State> constexpr return_type operator()(lexy::nullopt, State &state) const
    {
        if constexpr (requires { state.getAllocator(); })
            return return_type(state.getAllocator());
        else
            return return_type(std::pmr::get_default_resource());
    }

    /**
     * Accumulates parsed elements into a container allocated from the associated memory resource.
     */
    struct _sink
    {
        using return_type = typename traits::container_type;
        return_type _cont;

        explicit _sink(std::pmr::memory_resource *mr) : _cont(mr ? mr : std::pmr::get_default_resource()) {}

        template <typename U> void operator()(U &&item)
        {
            if constexpr (!std::is_same_v<std::decay_t<U>, lexy::nullopt>)
            {
                _cont.push_back(std::forward<U>(item));
            }
        }

        return_type finish() && { return std::move(_cont); }
    };

    template <typename State> _sink sink(State &state) const
    {
        if constexpr (requires { state.getAllocator(); })
        {
            return _sink(state.getAllocator());
        }
        else
        {
            return _sink(std::pmr::get_default_resource());
        }
    }

    _sink sink() const { return _sink(std::pmr::get_default_resource()); }
};

/**
 * Global constant helper instance to bind Lexy list rules to PMR arena allocation sinks.
 */
template <typename ValueType> constexpr PmrListSink<ValueType> PmrAsList{};

} // namespace DSL::Parser::Common

#endif // EZDSL_COMMON_PARSERS_H