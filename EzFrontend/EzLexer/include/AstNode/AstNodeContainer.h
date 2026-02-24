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
     * Returns true if this container hast at least 1 expression.
     * @return bool
     */
    bool containsExpressions() const;

    /**
     * Adds an expression to the container.
     * @param expression
     */
    void addExpression(const std::shared_ptr<AstNode> &expression);

    /**
     * Erases the given set of elements out of the expression list. If any key is invalid, an exception is thrown.
     *
     * IMPORTANT: If called inside a for loop will CRASH the program due to iterator invalidation. If you want
     * to delete a set / single element(s) save the indexes to be deleted. And ONLY delete them after every iteration
     * has been finished.
     * @param keys
     */
    void eraseExpression(std::list<size_t> keys);

    /**
     * Returns the expression linked to the given key. If no expression has been found, nullptr is returned.
     * @param key
     * @return std::shared_ptr<AstNode>
     */
    std::shared_ptr<AstNode> getExpressionAtIndex(size_t key) const;

    /**
     * Returns the expressions defined in this container.
     * @return const std::map<size_t, std::shared_ptr<AstNode>> &
     */
    const std::map<size_t, std::shared_ptr<AstNode>> &getExpressions() const;

  private:
    size_t currentId{ 0 };
    std::map<size_t, std::shared_ptr<AstNode>> m_expressions;
};

#endif // EZPACKER_ASTNODECONTAINER_H
