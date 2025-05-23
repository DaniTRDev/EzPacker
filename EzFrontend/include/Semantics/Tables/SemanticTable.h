#ifndef EZPACKER_SEMANTICTABLE_H
#define EZPACKER_SEMANTICTABLE_H

#include "EzFrontendCommon.h"

/**
 * Helper template used to generate tables of a certain type.
 * @tparam T
 */
template <typename T> class SemanticTable
{
  public:
    virtual ~SemanticTable() = default;

    /**
     * Adds a new element, if possible, to CURRENT scope and returns its ID. If element is already present, 0 is
     * returned. This function will check ONLY in this scope.
     * @param elem
     * @param name
     * @return bool
     */
    bool addElement(const std::shared_ptr<T> &data, std::string name)
    {
        if (doesElemExist(name))
            return false;

        m_table.top().insert({name, data});
        return true;
    }

    /**
     * Returns true if the element exists. If no scope is present, it will also return false. This function ONLY checks
     * this scope.
     * @param name
     * @return bool
     */
    bool doesElemExist(const std::string &name) const
    {
        return m_table.top().contains(name);
    }

    bool doesElemExistInAnyScope(const std::string &name) const
    {
        bool found = false;
        std::stack<std::unordered_map<std::string, std::shared_ptr<T>>> copy;
        
        while(!m_table.empty() && !found)
        {
            std::unordered_map<std::string, std::shared_ptr<T>> &subTable = std::move(m_table.top());
            m_table.pop();
            
            if (subTable.contains(name))
            {
                found = true;
                break;
            }
            
            copy.push(std::move(subTable));
        }
        
        // Restore table.
        while(!copy.empty())
        {
            m_table.emplace(std::move(copy.top()));
            copy.pop();
        }
        
        return found;
    }
    
    /**
     * Begins a scope, each scope has its own items.
     */
    void beginScope()
    {
        m_table.push({});
    }

    /**
     * Ends a scope.
     */
    void endScope()
    {
        m_table.pop();
    }

    /**
     * Returns the element. If it doesn't exist or is 0, nullptr is returned.
     * @param id
     * @return std::shared_ptr<T>
     */
    std::shared_ptr<T> getElement(const std::string &name)
    {
        if (!doesElemExist(name))
            return nullptr;

        return m_table.top().at(name);
    }

  private:
    std::stack<std::unordered_map<std::string, std::shared_ptr<T>>> m_table;
};

#endif // EZPACKER_SEMANTICTABLE_H
