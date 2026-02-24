#include "AstNode/AstNodeContainer.h"

bool AstNodeContainer::containsExpressions() const { return !m_expressions.empty(); }

void AstNodeContainer::addExpression(const std::shared_ptr<AstNode> &expression)
{
    m_expressions.insert({ currentId++, expression });
}

void AstNodeContainer::eraseExpression(std::list<size_t> keys)
{
    while (!keys.empty())
    {
        uint64_t key = keys.front();
        keys.pop_front();

        auto it = m_expressions.find(key);
        if (it == m_expressions.end())
        {
            throw std::runtime_error("Tried to delete an expression with invalid key");
        }

        m_expressions.erase(it);
    }
}

std::shared_ptr<AstNode> AstNodeContainer::getExpressionAtIndex(size_t key) const
{
    auto it = m_expressions.find(key);

    if (it == m_expressions.end())
        return nullptr;

    return it->second;
}

const std::map<size_t, std::shared_ptr<AstNode>> &AstNodeContainer::getExpressions() const { return m_expressions; }
