#ifndef EZPACKER_SCOPEDTABLE_H
#define EZPACKER_SCOPEDTABLE_H

#include "EzAnnotatorCommon.h"

/**
 * Class used to ensure that elements that will be saved in a "ScopedTable" can be assigned an unique id. Even if
 * two elements return the same at "getString", and they are in the same scope, they will have different UNIQUE IDs.
 */
class IndexableScopedItem
{
  public:
    virtual ~IndexableScopedItem() = default;

    /**
     * Returns the string representation of this element.
     * @return const std::string &
     */
    virtual const std::string &getString() = 0;
};

/**
 * Class that generates a single ID, unique in ALL of the scopes.
 */
class UniqueScopedItemIdGenerator
{
  public:
    /**
     * Generates an unique id for the given item. If the item is null, 0 is returned.
     * @param item
     * @param currentScopeId
     * @return size_t
     */
    inline static size_t generate(IndexableScopedItem *item, size_t currentScopeId)
    {
        if (!item)
            return 0;

        std::string uniqueStr = std::to_string(m_currentId++)
                                        .append("_")
                                        .append(std::to_string(currentScopeId))
                                        .append("_")
                                        .append(item->getString());
        return std::hash<std::string>{}(uniqueStr);
    }

  private:
    inline static size_t m_currentId{ 0 }; // Incremented after each call to generate.
};

/**
 * This object acts as a base scope-manager that can store information. It will be used to create symbol and type
 * tables. It uses a level-logic to ensure that scopes are kept alive when they are finished (endScope) without
 * interfering with unfinished scopes.
 * @tparam TableDataT
 */
template <typename TableDataT>
    requires(std::is_base_of_v<IndexableScopedItem, TableDataT>)
class ScopedTable
{
  public:
    using TableT = std::map<size_t, std::shared_ptr<TableDataT>>;
    struct ScopeT
    {
        size_t m_level;
        TableT m_table;
    };

    /**
     * Creates the table with current and max nesting level set to 0.
     */
    ScopedTable() : m_currentLevel(0), m_maxLevel(0) {}

    /**
     * Checks if given item ID exists in the current scope. If returns true ONLY if element exists. If current level
     * is 0 (there aren't opened scoped) it will return false.
     * @param id
     * @return bool
     */
    bool doesElemExist(size_t id)
    {
        if (m_currentLevel == 0)
            return false;

        for (auto scope = m_scopes.rbegin(); scope != m_scopes.rend(); scope++)
        {
            if (scope->m_level == m_currentLevel)
                return scope->m_table.contains(id);
        }
        return false;
    }

    /**
     * Tries recover an item by its STRING at ANY SCOPE, starting from the LAST-LEVEL scope with bottom-upper order.
     * Will return false if item does not exist or if current level is 0 (there aren't opened scopes) it will return
     * false.
     * @param string
     * @param outId
     * @return bool
     */
    bool doesElemExistAtAnyScopeByString(const std::string &string, size_t &outId)
    {
        if (m_currentLevel == 0)
            return false;

        size_t checkedScopeLevel = m_maxLevel;
        while (checkedScopeLevel != 0)
        {
            for (auto scope = m_scopes.rbegin(); scope != m_scopes.rend(); scope++)
            {
                if (scope->m_level == checkedScopeLevel)
                {
                    for (auto &[id, item] : scope->m_table)
                    {
                        if (item->getString() == string)
                        {
                            outId = id;
                            return true;
                        }
                    }
                }
            }

            checkedScopeLevel--;
        }
        return false;
    }

    /**
     * Checks if given item string exists in the current scope and returns its ID. If current level is 0 (there aren't
     * opened scoped) it will return false. If there are multiple items with the same string in this node, the first
     * one (based on the ID) is returned, so there's no guarantee of the order of duplicated elements.
     * @param string
     * @param outId
     * @return bool
     */
    bool doesElemExistByString(const std::string &string, size_t &outId)
    {
        if (m_scopes.empty())
            false;

        for (auto scope = m_scopes.rbegin(); scope != m_scopes.rend(); scope++)
        {
            if (scope->m_level == m_currentLevel)
            {
                for (auto &[id, item] : scope->m_table)
                {
                    if (item->getString() == string)
                    {
                        outId = id;
                        return true;
                    }
                }
            }
        }
        return false;
    }

    /**
     * Checks if given item ID exists in any scope, bottom-upper order starting from the current one. If current level
     * is 0 (there aren't opened scoped) it will return false.
     * @param id
     * @return bool
     */
    bool doesElemExistAtAnyUpperScope(size_t id)
    {
        if (m_currentLevel == 0)
            return false;

        size_t checkedScopeLevel = m_currentLevel;
        while (checkedScopeLevel != 0)
        {
            for (auto scope = m_scopes.rbegin(); scope != m_scopes.rend(); scope++)
            {
                if (scope->m_level == checkedScopeLevel)
                {
                    if (scope->m_table.contains(id))
                        return true;
                }
            }

            checkedScopeLevel--;
        }
        return false;
    }

    /**
     * Checks if given item string exists in the any upper scope and returns its ID. If there isn't any scope, an
     * exception will be thrown. If there are multiple items with the same string in this node, the first
     * one (based on the ID) is returned, so there's no guarantee of the order of duplicated elements.
     * @param string
     * @param outId
     * @return bool
     */
    bool doesElemExistAtAnyUpperScopeByString(const std::string &string, size_t &outId)
    {
        if (m_currentLevel == 0)
            throw std::runtime_error("Can't check if item exists because there isn't any scope");

        size_t checkedScopeLevel = m_currentLevel;
        while (checkedScopeLevel != 0)
        {
            for (auto scope = m_scopes.rbegin(); scope != m_scopes.rend(); scope++)
            {
                if (scope->m_level == checkedScopeLevel)
                {
                    for (auto &[id, item] : scope->m_table)
                    {
                        if (item->getString() == string)
                        {
                            outId = id;
                            return true;
                        }
                    }
                }
            }

            checkedScopeLevel--;
        }
        return false;
    }

    /**
     * Tries to add an item to the current scope, if there isn't any scope 0 is returned. Returns the ID
     * of the item. Duplicated items are also inserted because their ID will be different.
     * @param type
     * @return bool
     */
    size_t addItem(const std::shared_ptr<TableDataT> &item)
    {
        if (m_currentLevel == 0)
            return 0;

        size_t id = UniqueScopedItemIdGenerator::generate(item.get(), m_scopes.size());
        for (auto scope = m_scopes.rbegin(); scope != m_scopes.rend(); scope++)
        {
            if (scope->m_level == m_currentLevel)
            {
                scope->m_table[id] = item;
            }
        }
        return id;
    }

    /**
     * Tries recover an item by its ID at the CURRENT SCOPE. Will return nullptr if there isn't any scope,
     * item does not exist or item can't be casted to T.
     * @tparam T
     * @param id
     * @return const std::shared_ptr<T> &
     */
    template <typename T> const std::shared_ptr<T> &getItem(size_t id)
    {
        for (auto scope = m_scopes.rbegin(); scope != m_scopes.rend(); scope++)
        {
            if (scope->m_level == m_currentLevel)
            {
                if (scope->m_table.contains(id))
                    return std::dynamic_pointer_cast<T>(scope->m_table.at(id));
            }
        }

        return nullptr;
    }

    /**
     * Same as template <typename T> std::shared_ptr<T> getItem(size_t id) but with T = TableDataT.
     * @param id
     * @return const std::shared_ptr<TableDataT> &
     */
    template <> const std::shared_ptr<TableDataT> &getItem(size_t id) { return getItem<TableDataT>(); }

    /**
     * Tries recover an item by its ID at ANY SCOPE, starting from the LAST-LEVEL scope, with bottom-upper order. Will
     * return nullptr if there isn't any scope, item does not exist or item can't be casted to T.
     * @tparam T
     * @param id
     * @return const std::shared_ptr<T> &
     */
    template <typename T> const std::shared_ptr<T> &getItemAtAnyScope(size_t id)
    {
        size_t checkedScopeLevel = m_maxLevel;
        while (checkedScopeLevel != 0)
        {
            for (auto scope = m_scopes.rbegin(); scope != m_scopes.rend(); scope++)
            {
                if (scope->m_level == checkedScopeLevel)
                {
                    if (scope->m_table.contains(id))
                        return std::static_pointer_cast<T>(scope->m_table.at(id));
                }
            }

            checkedScopeLevel--;
        }
        return nullptr;
    }

    /**
     * Same as template <typename T> const std::shared_ptr<T> &getItemAtAnyScope(size_t id) but with T = TableDataT.
     * @param id
     * @return const std::shared_ptr<TableDataT> &
     */
    template <> const std::shared_ptr<TableDataT> &getItemAtAnyScope(size_t id)
    {
        return getItemAtAnyScope<TableDataT>(id);
    }
    
    /**
     * Tries recover an item by its ID at ANY SCOPE, bottom-upper order. Will return nullptr if there isn't any scope,
     * item does not exist or item can't be casted to T.
     * @tparam T
     * @param id
     * @return const std::shared_ptr<T> &
     */
    template <typename T> std::shared_ptr<T> getItemAtAnyUpperScope(size_t id)
    {
        size_t checkedScopeLevel = m_currentLevel;
        while (checkedScopeLevel != 0)
        {
            for (auto scope = m_scopes.rbegin(); scope != m_scopes.rend(); scope++)
            {
                if (scope->m_level == checkedScopeLevel)
                {
                    if (scope->m_table.contains(id))
                        return std::static_pointer_cast<T>(scope->m_table.at(id));
                }
            }

            checkedScopeLevel--;
        }
        return nullptr;
    }

    /**
     * Same as template <typename T> std::shared_ptr<T> getItemAtAnyUpperScope(size_t id) but with T = TableDataT.
     * returning type.
     * @param id
     * @return const std::shared_ptr<TableDataT> &
     */
    template <> std::shared_ptr<TableDataT> getItemAtAnyUpperScope(size_t id)
    {
        return getItemAtAnyUpperScope<TableDataT>(id);
    }

    /**
     * Begins a new scope, advances current level and sets max if current level value is greater than last max.
     */
    void beginScope()
    {
        m_scopes.emplace_back(++m_currentLevel, TableT{});

        if (m_currentLevel > m_maxLevel)
            m_maxLevel++;
    }

    /**
     * Finishes the current scope and decrements current level. If there aren't any opened scopes, an exception is
     * thrown.
     */
    void endScope()
    {
        if (m_currentLevel == 0)
            throw std::runtime_error("Can't call endScope without calling beginScope first");

        m_currentLevel--;
    }

  private:
    size_t m_currentLevel;
    size_t m_maxLevel;
    std::vector<ScopeT> m_scopes;
};

#endif // EZPACKER_SCOPEDTABLE_H
