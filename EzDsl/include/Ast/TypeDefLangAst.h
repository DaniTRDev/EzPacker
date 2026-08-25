#ifndef EZDSL_TYPE_DESCRIPTOR_AST_H
#define EZDSL_TYPE_DESCRIPTOR_AST_H

#include "EzDslCommon.h"
#include "CommonAstNodes.h"

namespace DSL::Ast::TypeDef
{

/**
 * Enumeration of primitive and special type categories definable in .tyf files.
 */
enum class TypeKind : uint8_t
{
    Integer,
    FloatingPoint,
    Void,
    BindingToken,
    Pointer
};

/**
 * AST node for primitive and special type declarations in .tyf files.
 *
 * Syntax:
 *   TypeDescriptor := TypeKindName TypeName ('(' BitWidth ')')? ';'
 *   TypeKindName   := 'integer' | 'float' | 'void' | 'bindingToken' | 'pointer'
 *   TypeName       := Identifier
 *   BitWidth       := IntegerLiteral
 *
 * Examples:
 *   integer i32(32);
 *   float f64(64);
 *   void void_t;
 *   bindingToken __token;
 *   pointer ptr(64);
 */
struct TypeDescriptor
{
    TypeKind m_kind;
    Common::Identifier m_name;
    std::optional<Common::IntegerLiteral> m_bitSize;
};

/**
 * Top-level AST root representing a parsed .tyf type definitions file.
 *
 * Syntax:
 *   TypeDefFile := ( TypeDescriptor )* EOF
 */
struct TypeDefFile
{
    std::pmr::vector<TypeDescriptor> m_types;
};

}; // namespace DSL::Ast::TypeDef

#endif // EZDSL_TYPE_DESCRIPTOR_AST_H