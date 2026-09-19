#ifndef EZDSLLEXER_TYPE_DESCRIPTOR_AST_H
#define EZDSLLEXER_TYPE_DESCRIPTOR_AST_H

#include "EzDslLexerCommon.h"
#include "CommonAstNodes.h"

namespace DSL::Ast::TypeDef
{

/**
 * Kind of a type declared in a `.tyf` type definition file.
 */
enum class TypeKind : uint8_t
{
    Integer,       // Integer scalar with an explicit bit width.
    FloatingPoint, // IEEE floating-point scalar (16/32/64/128 bits).
    Void,          // Void/no-value type.
    BindingToken,  // Opaque token used only to bind operands.
    Pointer        // Target pointer type.
};

/**
 * A single declared type: its kind, name, optional bit size, and optional explicit alignment.
 */
struct TypeDescriptor
{
    TypeKind m_kind;                                   // Type category.
    Common::Identifier m_name;                         // Type name.
    std::optional<Common::IntegerLiteral> m_bitSize;   // Width in bits (required for scalar kinds).
    std::optional<Common::IntegerLiteral> m_alignment; // Explicit alignment; defaults to bit size.
};

/**
 * Root AST node for a parsed `.tyf` type definition file.
 */
struct TypeDefFile
{
    std::pmr::vector<TypeDescriptor> m_types; // All declared types.
};

}; // namespace DSL::Ast::TypeDef

#endif // EZDSLLEXER_TYPE_DESCRIPTOR_AST_H