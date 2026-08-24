#ifndef EZDSL_TYPE_DESCRIPTOR_AST_H
#define EZDSL_TYPE_DESCRIPTOR_AST_H

#include "EzDslCommon.h"
#include "CommonAstNodes.h"

namespace DSL::Ast::TypeDef
{

enum class TypeKind : uint8_t
{
    Integer,
    FloatingPoint
};

/**
 * Defines an IR PRIMITIVE type: integer i8(8).
 */
struct TypeDescriptor
{
    TypeKind m_kind;
    Common::Identifier m_name;
    Common::IntegerLiteral m_bitSize;
};

/**
 * Types defined in a type definition file.
 */
struct TypeDefFile
{
    std::pmr::vector<TypeDescriptor> m_types;
};

}; // namespace DSL::Ast::TypeDef

#endif // EZDSL_TYPE_DESCRIPTOR_AST_H