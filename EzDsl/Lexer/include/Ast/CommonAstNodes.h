#ifndef EZDSLLEXER_COMMON_AST_NODES_H
#define EZDSLLEXER_COMMON_AST_NODES_H

#include "EzDslLexerCommon.h"

class SourceReference;

namespace DSL::Ast::Common
{

/**
 * Generic container wrapping an AST payload with its originating SourceReference span.
 * Enables zero-copy tracking of source code intervals during parsing and semantic analysis.
 */
template <typename Node> struct SourcedAstNode
{
    Node m_node;                             // The actual parsed payload value.
    SourceReference *m_sourceRef{ nullptr }; // Source span covering the parsed token(s).
};

/**
 * A signed decimal integer literal token.
 */
struct IntegerLiteral : SourcedAstNode<int64_t>
{
};

/**
 * A bare identifier token, stored as a view into the source buffer.
 */
struct Identifier : SourcedAstNode<std::string_view>
{
};

/**
 * A double-quoted string literal token, stored as a view into the source buffer.
 */
struct StringLiteral : SourcedAstNode<std::string_view>
{
};

/**
 * A case-insensitive boolean literal token ('true'/'false').
 */
struct BooleanLiteral : SourcedAstNode<bool>
{
};

} // namespace DSL::Ast::Common

#endif // EZDSLLEXER_COMMON_AST_NODES_H