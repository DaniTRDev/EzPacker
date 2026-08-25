#ifndef EZDSL_TYPE_DEF_LANG_H
#define EZDSL_TYPE_DEF_LANG_H

#include "EzDslCommon.h"
#include "Ast/TypeDefLangAst.h"
#include "Parser/CommonParsers.h"

namespace DSL::Parser::TypeDef
{
namespace dsl = ::lexy::dsl;

/**
 * Lexy symbol table parser mapping type classification keywords to Ast::TypeDef::TypeKind enum values.
 *
 * Syntax:
 *   TypeKind := 'integer' | 'float' | 'void' | 'bindingToken' | 'pointer'
 */
struct TypeKind
{
    static constexpr auto Table =
            lexy::symbol_table<Ast::TypeDef::TypeKind>
                .map(LEXY_LIT("integer"), Ast::TypeDef::TypeKind::Integer)
                .map(LEXY_LIT("float"), Ast::TypeDef::TypeKind::FloatingPoint)
                .map(LEXY_LIT("void"), Ast::TypeDef::TypeKind::Void)
                .map(LEXY_LIT("bindingToken"), Ast::TypeDef::TypeKind::BindingToken)
                .map(LEXY_LIT("pointer"), Ast::TypeDef::TypeKind::Pointer);

    static constexpr auto rule = dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha_underscore));
    static constexpr auto value = lexy::forward<Ast::TypeDef::TypeKind>;
};

/**
 * Lexy parser rule for a single type definition statement.
 *
 * Syntax:
 *   TypeDescriptor := TypeKind Identifier ( '(' IntegerLiteral ')' )?
 *
 * Examples:
 *   integer i32(32)
 *   float f64(64)
 *   void void_t
 */
struct TypeDescriptor
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<TypeKind> + dsl::p<Common::Identifier> +
            dsl::opt(dsl::parenthesized(dsl::p<Common::IntegerLiteral>));
    static constexpr auto value = lexy::construct<Ast::TypeDef::TypeDescriptor>;
};

/**
 * Top-level Lexy file parser for .tyf type definition files.
 *
 * Syntax:
 *   TypeDefFile := ( TypeDescriptor ';' )*
 */
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