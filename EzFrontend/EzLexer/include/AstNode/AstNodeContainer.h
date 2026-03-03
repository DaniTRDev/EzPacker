#ifndef EZPACKER_ASTNODECONTAINER_H
#define EZPACKER_ASTNODECONTAINER_H

#include "EzLexerCommon.h"
#include "AstNode.h"

/**
 * Class that makes an AstNode be able to store other AstNode. It also provides methods for easy AST manipulation
 * (removing nodes, adding nodes, ...).
 */
class AstNodeContainer
{
  public:
    /**
     * Returns true if this container has at least 1 expression. Requires that expressions have been set
     * (via setExpressions) prior to calling.
     * @return bool
     */
    bool containsExpressions() const;

    /**
     * Returns the count of expressions. Assumes expressions have been set (via setExpressions); calling
     * this when expressions are unset results in undefined behavior.
     * @return size_t
     */
    size_t getExpressionCount() const;

    /**
     * Returns the expressions defined in this container (as a slice of a TypedPool). May return nullptr
     * if no expressions have been set.
     * @return TypedPoolSlice<AstNode> *
     */
    TypedPoolSlice<AstNode> *getExpressions() const;

    /**
     * Sets the expressions of this container.
     * @param expressions
     */
    void setExpressions(TypedPoolSlice<AstNode> *expressions);

  private:
    TypedPoolSlice<AstNode> *m_expressions{ nullptr };
};

#endif // EZPACKER_ASTNODECONTAINER_H
