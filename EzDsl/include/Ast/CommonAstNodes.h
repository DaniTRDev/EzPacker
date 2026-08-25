#ifndef EZDSL_COMMON_AST_NODES_H
#define EZDSL_COMMON_AST_NODES_H

#include "EzDslCommon.h"
#include <optional>
#include <string_view>

class SourceReference;

namespace DSL::Ast::Common
{

/**
 * Generic container wrapping an AST payload with its originating SourceReference span.
 * Enables zero-copy tracking of source code intervals during parsing and semantic analysis.
 */
template <typename Node> struct SourcedAstNode
{
    Node m_node;
    SourceReference *m_sourceRef{ nullptr };
};

/**
 * AST node for parsed integer literals.
 *
 * Syntax:
 *   IntegerLiteral := ['+' | '-']? ( DecLiteral | HexLiteral | BinLiteral | OctLiteral )
 *   DecLiteral     := [0-9]+
 *   HexLiteral     := ('0x' | '0X') [0-9a-fA-F]+
 *   BinLiteral     := ('0b' | '0B') [01]+
 *   OctLiteral     := ('0o' | '0O') [0-7]+
 */
struct IntegerLiteral : SourcedAstNode<int64_t>
{
};

/**
 * AST node for parsed floating-point real literals.
 *
 * Syntax:
 *   RealLiteral := ['+' | '-']? [0-9]+ '.' [0-9]+ ( ('e' | 'E') ['+' | '-']? [0-9]+ )?
 */
struct RealLiteral : SourcedAstNode<double>
{
};

/**
 * AST node for alphanumeric identifiers.
 *
 * Syntax:
 *   Identifier := [a-zA-Z_] [a-zA-Z0-9_]*
 */
struct Identifier : SourcedAstNode<std::string_view>
{
};

/**
 * AST node for double-quoted string literals.
 *
 * Syntax:
 *   StringLiteral := '"' ( [^"\\] | '\\' . )* '"'
 */
struct StringLiteral : SourcedAstNode<std::string_view>
{
};

/**
 * AST node representing a type-annotated identifier with optional parameterized type arguments.
 *
 * Syntax:
 *   TypedIdentifier := TypeName ('(' TypeParam ')')? ':' Identifier
 *   TypeName        := Identifier
 *   TypeParam       := Identifier
 *
 * Examples:
 *   - "GPR:rd"          -> m_type = "GPR",  m_typeParam = std::nullopt, m_name = "rd"
 *   - "simm(i12):imm12" -> m_type = "simm", m_typeParam = "i12",        m_name = "imm12"
 */
struct TypedIdentifier
{
    Identifier m_type;
    std::optional<Identifier> m_typeParam;
    Identifier m_name;

    /**
     * Checks if the type identifier corresponds to an immediate operand classifier ("imm", "simm", "uimm").
     */
    [[nodiscard]] bool isImmediate() const noexcept
    {
        return m_type.m_node == "imm" || m_type.m_node == "simm" || m_type.m_node == "uimm";
    }
};

} // namespace DSL::Ast::Common

#endif // EZDSL_COMMON_AST_NODES_H