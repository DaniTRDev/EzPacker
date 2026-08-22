#ifndef EZDSL_COMMON_AST_NODES_H
#define EZDSL_COMMON_AST_NODES_H

#include "EzDslCommon.h"
#include <optional>
#include <string_view>

class SourceReference;

namespace DSL::Ast::Common
{

/**
 * Base wrapper attaching source location metadata to AST values.
 */
template <typename Node> struct SourcedAstNode
{
    Node m_node;
    SourceReference *m_sourceRef{ nullptr };
};

struct IntegerLiteral : SourcedAstNode<int64_t>
{
};
struct RealLiteral : SourcedAstNode<double>
{
};
struct Identifier : SourcedAstNode<std::string_view>
{
};
struct StringLiteral : SourcedAstNode<std::string_view>
{
};

/**
 * Unified representation for typed identifiers across DSLs:
 *   - "GPR:rd"          -> type="GPR",  param=nullopt, name="rd"
 *   - "simm(i12):imm12" -> type="simm", param="i12",   name="imm12"
 */
struct TypedIdentifier
{
    Identifier m_type;
    std::optional<Identifier> m_typeParam;
    Identifier m_name;

    [[nodiscard]] bool isImmediate() const noexcept
    {
        return m_type.m_node == "imm" || m_type.m_node == "simm" || m_type.m_node == "uimm";
    }
};

} // namespace DSL::Ast::Common

#endif // EZDSL_COMMON_AST_NODES_H