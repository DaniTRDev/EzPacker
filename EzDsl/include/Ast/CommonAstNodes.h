#ifndef EZDSL_COMMON_AST_NODES_H
#define EZDSL_COMMON_AST_NODES_H

#include "EzDslCommon.h"

// Forward declare this.
class SourceReference;

namespace DSL::Ast::Common
{

/**
 * Common type for every literal node. It contains the node type and the source reference.
 */
template <typename Node> struct SourcedAstNode
{
    Node m_node;
    SourceReference *m_sourceRef;
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

} // namespace DSL::Ast::Common

#endif // EZDSL_COMMON_AST_NODES_H
