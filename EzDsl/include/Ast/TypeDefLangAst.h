#ifndef EZDSL_TYPE_DESCRIPTOR_AST_H
#define EZDSL_TYPE_DESCRIPTOR_AST_H

#include "EzDslCommon.h"
#include "CommonAstNodes.h"

namespace DSL::Ast::TypeDef
{

enum class TypeKind : uint8_t
{
    Integer,
    FloatingPoint,
    Void,
    BindingToken
};

/**
 * Defines an IR PRIMITIVE or SPECIAL type:
 * - integer i8(8);
 * - float f32(32);
 * - void void; or void void(0);
 * - bindingToken __bindToken;
 */
struct TypeDescriptor
{
    TypeKind m_kind;
    Common::Identifier m_name;
    std::optional<Common::IntegerLiteral> m_bitSize;
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