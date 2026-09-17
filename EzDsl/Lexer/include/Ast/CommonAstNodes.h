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

struct BooleanLiteral : SourcedAstNode<bool>
{
};

} // namespace DSL::Ast::Common

#endif // EZDSLLEXER_COMMON_AST_NODES_H