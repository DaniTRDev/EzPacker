#ifndef EZDSL_TYPE_DEF_LANG_H
#define EZDSL_TYPE_DEF_LANG_H

#include "EzDslCommon.h"
#include "Ast/TypeDefLangAst.h"
#include "Parser/CommonParsers.h"

namespace DSL::Parser::TypeDef
{
namespace dsl = ::lexy::dsl;

struct TypeKind
{
    static constexpr auto Table =
            lexy::symbol_table<Ast::TypeDef::TypeKind>
                .map(LEXY_LIT("integer"), Ast::TypeDef::TypeKind::Integer)
                .map(LEXY_LIT("float"), Ast::TypeDef::TypeKind::FloatingPoint)
                .map(LEXY_LIT("void"), Ast::TypeDef::TypeKind::Void)
                .map(LEXY_LIT("bindingToken"), Ast::TypeDef::TypeKind::BindingToken)
                .map(LEXY_LIT("binding_token"), Ast::TypeDef::TypeKind::BindingToken);

    static constexpr auto rule = dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha_underscore));
    static constexpr auto value = lexy::forward<Ast::TypeDef::TypeKind>;
};

struct TypeDescriptor
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<TypeKind> + dsl::p<Common::Identifier> +
            dsl::opt(dsl::parenthesized(dsl::p<Common::IntegerLiteral>));
    static constexpr auto value = lexy::construct<Ast::TypeDef::TypeDescriptor>;
};

struct TypeDefFile
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule =
            dsl::list(dsl::peek(dsl::ascii::alpha_underscore) >> (dsl::p<TypeDescriptor> + dsl::lit_c<';'>));
    static constexpr auto value =
            Common::PmrAsList<Ast::TypeDef::TypeDescriptor> >> lexy::construct<Ast::TypeDef::TypeDefFile>;
};

}; // namespace DSL::Parser::TypeDef

#endif // EZDSL_TYPE_DEF_LANG_H