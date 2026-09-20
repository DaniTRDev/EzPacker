#ifndef EZDSL_PARSER_ENCODING_DEF_LANG_H
#define EZDSL_PARSER_ENCODING_DEF_LANG_H

#include "Ast/CommonAstNodes.h"
#include "Ast/EncodingDefLangAst.h"
#include "EzDslLexerCommon.h"
#include "Parser/CommonParsers.h"

namespace DSL::Parser::Encoding
{
namespace dsl = ::lexy::dsl;
namespace AstEnc = ::DSL::Ast::Encoding;

/**
 * Parses a single raw byte literal (decimal or 0x-prefixed hex).
 */
struct ByteLiteral
{
    static constexpr auto rule = []
    {
        auto hex = (dsl::lit<"0x"> | dsl::lit<"0X">) >> dsl::integer<uint8_t, dsl::hex>(dsl::digits<dsl::hex>);
        auto dec = dsl::integer<uint8_t>(dsl::digits<dsl::decimal>);
        return hex | dec;
    }();
    static constexpr auto value = lexy::forward<uint8_t>;
};

/**
 * Parses a `name => field` operand binding inside a generic ENCODING block.
 */
struct OperandBindingParser
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<Common::Identifier> + LEXY_LIT("=>") + dsl::p<Common::Identifier>;
    static constexpr auto value = lexy::callback<AstEnc::OperandBinding>(
            [](Ast::Common::Identifier operand, Ast::Common::Identifier field)
            { return AstEnc::OperandBinding{ .m_operand = std::move(operand), .m_field = std::move(field) }; });
};

/**
 * Parses the generic, target-defined value following a directive key.
 *
 * Shapes: `[bytes]`, `operands { op => field; ... }`, a boolean, an integer, or an
 * identifier. The parser has no architecture knowledge; keys and fields are accepted
 * as-is.
 */
struct ValueParser
{
    static constexpr auto whitespace = Common::Whitespace;

    // `[0x0F, 0x58]`
    struct ByteList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::square_bracketed.list(dsl::p<ByteLiteral>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<uint8_t>>;
    };

    // `{ name => field; ... }`
    struct OperandBindings
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<OperandBindingParser> + dsl::lit_c<';'>);
        static constexpr auto value = Common::PmrAsList<AstEnc::OperandBinding>;
    };

    struct Boolean
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<Common::BooleanLiteral>;
        static constexpr auto value =
                lexy::callback<AstEnc::Value>([](Ast::Common::BooleanLiteral v) -> AstEnc::Value { return v.m_node; });
    };

    struct Integer
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<Common::IntegerLiteral>;
        static constexpr auto value =
                lexy::callback<AstEnc::Value>([](Ast::Common::IntegerLiteral v) -> AstEnc::Value { return v.m_node; });
    };

    struct Identifier
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<Common::Identifier>;
        static constexpr auto value = lexy::callback<AstEnc::Value>([](Ast::Common::Identifier id) -> AstEnc::Value
                                                                    { return AstEnc::Value{ std::move(id) }; });
    };

    static constexpr auto rule = (dsl::peek(dsl::lit_c<'['>) >> dsl::p<ByteList>) |
            (dsl::peek(dsl::lit_c<'{'>) >> dsl::p<OperandBindings>) |
            (dsl::peek(Common::Keyword<"true">::rule | Common::Keyword<"false">::rule) >> dsl::p<Boolean>) |
            (dsl::peek(dsl::lit_c<'-'> | dsl::ascii::digit) >> dsl::p<Integer>) |
            (dsl::peek(dsl::ascii::alpha_underscore) >> dsl::p<Identifier>);

    static constexpr auto value = lexy::forward<AstEnc::Value>;
};

/**
 * Parses one `key : value` directive.
 *
 * The colon is optional so the block-valued `operands { ... }` form keeps its
 * existing surface syntax while every scalar directive uses `key : value`.
 */
struct DirectiveParser
{
    static constexpr auto whitespace = Common::Whitespace;

    struct KeyAndValue
    {
        struct WithColon
        {
            static constexpr auto rule = dsl::peek(dsl::lit_c<':'>) >> (dsl::lit_c<':'> >> dsl::p<ValueParser>);
            static constexpr auto value = lexy::forward<AstEnc::Value>;
        };
        struct NoColon
        {
            // Only the block-valued `operands { ... }` form omits the colon.
            static constexpr auto rule = dsl::peek(dsl::lit_c<'{'>) >> dsl::p<ValueParser>;
            static constexpr auto value = lexy::forward<AstEnc::Value>;
        };

        static constexpr auto rule = dsl::p<WithColon> | dsl::p<NoColon>;
        static constexpr auto value = lexy::forward<AstEnc::Value>;
    };

    static constexpr auto rule = dsl::p<Common::Identifier> + dsl::p<KeyAndValue>;
    static constexpr auto value = lexy::callback<AstEnc::Directive>(
            [](Ast::Common::Identifier key, AstEnc::Value value)
            { return AstEnc::Directive{ .m_key = std::move(key), .m_value = std::move(value) }; });
};

/**
 * Parses a generic `ENCODING [backend] { ... }` block.
 *
 * `TargetInstDefLang.h` reuses this production so the `.idf` surface syntax is
 * unchanged while the shared AST stays architecture-neutral.
 */
struct EncodingDeclParser
{
    static constexpr auto whitespace = Common::Whitespace;

    struct Backend
    {
        // No own whitespace: a production used as a branch (`dsl::opt`) must not define one.
        static constexpr auto rule = dsl::peek(dsl::lit_c<'['>) >> dsl::square_bracketed(dsl::p<Common::Identifier>);
        static constexpr auto value = lexy::forward<Ast::Common::Identifier>;
    };

    struct Body
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<DirectiveParser> + dsl::lit_c<';'>);
        static constexpr auto value = Common::PmrAsList<AstEnc::Directive>;
    };

    static constexpr auto rule = Common::Keyword<"ENCODING">::rule >>
            (dsl::opt(dsl::p<Backend>) + dsl::p<Body> + dsl::opt(dsl::lit_c<';'>));

    static constexpr auto value = lexy::callback<AstEnc::EncodingDecl>(
            [](auto backendOpt, std::pmr::vector<AstEnc::Directive> directives, auto...)
            {
                AstEnc::EncodingDecl decl;
                if constexpr (!std::is_same_v<std::decay_t<decltype(backendOpt)>, lexy::nullopt>)
                {
                    decl.m_backend = std::move(backendOpt);
                }
                decl.m_directives = std::move(directives);
                return decl;
            });
};

} // namespace DSL::Parser::Encoding

#endif // EZDSL_PARSER_ENCODING_DEF_LANG_H
