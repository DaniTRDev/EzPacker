#ifndef EZDSLLEXER_TYPE_DESCRIPTOR_AST_H
#define EZDSLLEXER_TYPE_DESCRIPTOR_AST_H

#include "EzDslLexerCommon.h"
#include "CommonAstNodes.h"

namespace DSL::Ast::TypeDef
{

enum class TypeKind : uint8_t
{
    Integer,
    FloatingPoint,
    Void,
    BindingToken,
    Pointer
};

struct TypeDescriptor
{
    TypeKind m_kind;
    Common::Identifier m_name;
    std::optional<Common::IntegerLiteral> m_bitSize;
    std::optional<Common::IntegerLiteral> m_alignment;
};

struct TypeDefFile
{
    std::pmr::vector<TypeDescriptor> m_types;
};

}; // namespace DSL::Ast::TypeDef

#endif // EZDSLLEXER_TYPE_DESCRIPTOR_AST_H