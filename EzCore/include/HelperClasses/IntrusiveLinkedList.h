#ifndef EZCORE_INTRUSIVE_LIST_H
#define EZCORE_INTRUSIVE_LIST_H

#include <cstddef>
#include <iterator>

/**
 * High-performance doubly-linked list that embeds linkage pointers directly inside element nodes.
 * Unlike standard library node-based containers (e.g. std::list or std::pmr::list), this intrusive design
 * incurs zero dynamic memory allocations on node insertion/removal operations.
 * Element nodes must implement getPrev(), setPrev(), getNext(), and setNext() accessor methods.
 */
template <typename T> class IntrusiveLinkedList
{
  public:
    /**
     * Bidirectional iterator for traversing and manipulating intrusive linked lists.
     */
    class iterator
    {
        friend class IntrusiveLinkedList;
        T *m_node{ nullptr };                         // Current node, or nullptr for the past-the-end sentinel.
        const IntrusiveLinkedList *m_list{ nullptr }; // Owning list, needed to resolve --end().

      public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T *;
        using difference_type = std::ptrdiff_t;
        using pointer = T **;
        using reference = T *&;

        /**
         * Creates the past-the-end sentinel iterator.
         */
        iterator() = default;
        /**
         * Binds the iterator to a node and the list that owns it.
         */
        iterator(T *node, const IntrusiveLinkedList *list) : m_node(node), m_list(list) {}

        /**
         * Dereferences to the underlying node pointer.
         */
        T *operator*() const { return m_node; }
        /**
         * Member-access through the underlying node pointer.
         */
        T *operator->() const { return m_node; }
        /**
         * Returns the raw node pointer held by the iterator.
         */
        T *get() const { return m_node; }

        /**
         * Prefix increment advancing iterator to the next node in sequence.
         */
        iterator &operator++()
        {
            if (m_node)
                m_node = m_node->getNext();
            return *this;
        }

        /**
         * Postfix increment advancing iterator to the next node and returning previous position.
         */
        iterator operator++(int)
        {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        /**
         * Prefix decrement moving iterator to the previous node or tail if at end.
         */
        iterator &operator--()
        {
            if (m_node)
                m_node = m_node->getPrev();
            else if (m_list)
                m_node = m_list->m_tail;
            return *this;
        }

        /**
         * Postfix decrement moving iterator to the previous node and returning previous position.
         */
        iterator operator--(int)
        {
            iterator tmp = *this;
            --(*this);
            return tmp;
        }

        /**
         * Equality compares the node positions, ignoring which list owns them.
         */
        bool operator==(const iterator &other) const { return m_node == other.m_node; }
        /**
         * Inequality compares the node positions, ignoring which list owns them.
         */
        bool operator!=(const iterator &other) const { return m_node != other.m_node; }
    };

    using const_iterator = iterator; // Iteration never mutates nodes, so const and mutable iterators match.
    using reverse_iterator = std::reverse_iterator<iterator>; // Reverse traversal built from the mutable iterator.
    using const_reverse_iterator =
            std::reverse_iterator<const_iterator>; // Reverse traversal built from the const iterator.

    /**
     * Creates an empty list.
     */
    IntrusiveLinkedList() = default;
    /**
     * Destroys the list; nodes are owned externally and are not freed here.
     */
    ~IntrusiveLinkedList() = default;

    IntrusiveLinkedList(const IntrusiveLinkedList &) = delete;
    IntrusiveLinkedList &operator=(const IntrusiveLinkedList &) = delete;

    /**
     * Move constructor transferring list ownership and resetting source list state.
     */
    IntrusiveLinkedList(IntrusiveLinkedList &&other) noexcept :
        m_head(other.m_head), m_tail(other.m_tail), m_size(other.m_size)
    {
        other.m_head = nullptr;
        other.m_tail = nullptr;
        other.m_size = 0;
    }

    /**
     * Move assignment operator transferring list pointers and size.
     */
    IntrusiveLinkedList &operator=(IntrusiveLinkedList &&other) noexcept
    {
        if (this != &other)
        {
            m_head = other.m_head;
            m_tail = other.m_tail;
            m_size = other.m_size;
            other.m_head = nullptr;
            other.m_tail = nullptr;
            other.m_size = 0;
        }
        return *this;
    }

    /**
     * Returns an iterator pointing to the first node in the list.
     */
    iterator begin() { return iterator(m_head, this); }

    /**
     * Returns an iterator representing the past-the-end sentinel.
     */
    iterator end() { return iterator(nullptr, this); }

    /**
     * Returns a const iterator pointing to the first node in the list.
     */
    const_iterator begin() const { return const_iterator(m_head, this); }

    /**
     * Returns a const iterator representing the past-the-end sentinel.
     */
    const_iterator end() const { return const_iterator(nullptr, this); }

    /**
     * Returns a const iterator to the first node.
     */
    const_iterator cbegin() const { return begin(); }
    /**
     * Returns a const past-the-end iterator.
     */
    const_iterator cend() const { return end(); }

    /**
     * Returns a reverse iterator starting at the tail (i.e. before rend()).
     */
    reverse_iterator rbegin() { return reverse_iterator(end()); }
    /**
     * Returns the reverse past-the-end iterator (before the head).
     */
    reverse_iterator rend() { return reverse_iterator(begin()); }
    /**
     * Returns a const reverse iterator starting at the tail.
     */
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    /**
     * Returns the const reverse past-the-end iterator.
     */
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

    /**
     * Returns true if the list contains zero elements.
     */
    bool empty() const { return m_size == 0; }

    /**
     * Returns the total count of elements currently linked in the list.
     */
    size_t size() const { return m_size; }

    /**
     * Returns a raw pointer to the first node in the list, or nullptr if empty.
     */
    T *front() const { return m_head; }

    /**
     * Returns a raw pointer to the last node in the list, or nullptr if empty.
     */
    T *back() const { return m_tail; }

    /**
     * Appends a pre-allocated node to the end of the intrusive list in O(1) time.
     */
    void push_back(T *node)
    {
        if (!node)
            return;
        node->setPrev(m_tail);
        node->setNext(nullptr);
        if (m_tail)
            m_tail->setNext(node);
        else
            m_head = node;
        m_tail = node;
        ++m_size;
    }

    /**
     * Prepends a pre-allocated node to the beginning of the intrusive list in O(1) time.
     */
    void push_front(T *node)
    {
        if (!node)
            return;
        node->setNext(m_head);
        node->setPrev(nullptr);
        if (m_head)
            m_head->setPrev(node);
        else
            m_tail = node;
        m_head = node;
        ++m_size;
    }

    /**
     * Inserts a pre-allocated node immediately before the position specified by iterator pos.
     */
    iterator insert(iterator pos, T *node)
    {
        if (!node)
            return pos;
        if (pos.m_node == nullptr)
        {
            push_back(node);
            return iterator(node, this);
        }
        if (pos.m_node == m_head)
        {
            push_front(node);
            return iterator(node, this);
        }

        T *curr = pos.m_node;
        T *prev = curr->getPrev();

        node->setPrev(prev);
        node->setNext(curr);
        if (prev)
            prev->setNext(node);
        curr->setPrev(node);

        ++m_size;
        return iterator(node, this);
    }

    /**
     * Unlinks the node referenced by iterator pos from the list without deallocating its memory.
     * Returns an iterator pointing to the element immediately following the removed node.
     */
    iterator erase(iterator pos)
    {
        if (!pos.m_node)
            return end();
        T *curr = pos.m_node;
        T *next = curr->getNext();
        T *prev = curr->getPrev();

        if (prev)
            prev->setNext(next);
        else
            m_head = next;

        if (next)
            next->setPrev(prev);
        else
            m_tail = prev;

        curr->setPrev(nullptr);
        curr->setNext(nullptr);
        --m_size;

        return iterator(next, this);
    }

    /**
     * Unlinks the specified node instance from the list in O(1) time.
     */
    void remove(T *node)
    {
        if (!node)
            return;
        erase(to_iterator(node));
    }

    /**
     * Unlinks and returns the first node in the list, or nullptr if empty.
     */
    T *pop_front()
    {
        if (empty())
            return nullptr;
        T *node = m_head;
        erase(begin());
        return node;
    }

    /**
     * Unlinks and returns the last node in the list, or nullptr if empty.
     */
    T *pop_back()
    {
        if (empty())
            return nullptr;
        T *node = m_tail;
        erase(to_iterator(m_tail));
        return node;
    }

    /**
     * Slices and transfers all elements from other list into this list before position pos in O(1) time.
     */
    void splice(iterator pos, IntrusiveLinkedList &other)
    {
        if (other.empty() || this == &other)
            return;

        T *first = other.m_head;
        T *last = other.m_tail;
        size_t count = other.m_size;

        other.m_head = nullptr;
        other.m_tail = nullptr;
        other.m_size = 0;

        if (empty())
        {
            m_head = first;
            m_tail = last;
            first->setPrev(nullptr);
            last->setNext(nullptr);
            m_size = count;
            return;
        }

        if (pos.m_node == nullptr)
        {
            m_tail->setNext(first);
            first->setPrev(m_tail);
            last->setNext(nullptr);
            m_tail = last;
            m_size += count;
            return;
        }

        if (pos.m_node == m_head)
        {
            last->setNext(m_head);
            m_head->setPrev(last);
            first->setPrev(nullptr);
            m_head = first;
            m_size += count;
            return;
        }

        T *curr = pos.m_node;
        T *prev = curr->getPrev();

        prev->setNext(first);
        first->setPrev(prev);
        last->setNext(curr);
        curr->setPrev(last);
        m_size += count;
    }

    /**
     * Slices and transfers a single element it from other list into this list before position pos in O(1) time.
     */
    void splice(iterator pos, IntrusiveLinkedList &other, iterator it)
    {
        if (!it.m_node)
            return;

        T *node = it.m_node;
        if (this == &other)
        {
            if (pos.m_node == node || (pos.m_node && pos.m_node == node->getNext()))
                return;
        }

        // Unlink node from other
        T *oPrev = node->getPrev();
        T *oNext = node->getNext();

        if (oPrev)
            oPrev->setNext(oNext);
        else
            other.m_head = oNext;

        if (oNext)
            oNext->setPrev(oPrev);
        else
            other.m_tail = oPrev;

        --other.m_size;
        node->setPrev(nullptr);
        node->setNext(nullptr);

        insert(pos, node);
    }

    /**
     * Slices and transfers range [first, last) from other list into this list before position pos in O(1) time.
     * Note: count must match the number of elements in [first, last).
     */
    void splice(iterator pos, IntrusiveLinkedList &other, iterator first, iterator last, size_t count)
    {
        if (count == 0 || !first.m_node || other.empty())
            return;

        if (first.m_node == other.m_head && last.m_node == nullptr && count == other.m_size)
        {
            splice(pos, other);
            return;
        }

        T *firstNode = first.m_node;
        T *lastNode = (last.m_node ? last.m_node->getPrev() : other.m_tail);
        if (!lastNode)
            return;

        // Unlink [firstNode, lastNode] from other
        T *oPrev = firstNode->getPrev();
        T *oNext = lastNode->getNext();

        if (oPrev)
            oPrev->setNext(oNext);
        else
            other.m_head = oNext;

        if (oNext)
            oNext->setPrev(oPrev);
        else
            other.m_tail = oPrev;

        other.m_size -= count;
        firstNode->setPrev(nullptr);
        lastNode->setNext(nullptr);

        if (empty())
        {
            m_head = firstNode;
            m_tail = lastNode;
            m_size = count;
            return;
        }

        if (pos.m_node == nullptr)
        {
            m_tail->setNext(firstNode);
            firstNode->setPrev(m_tail);
            m_tail = lastNode;
            m_size += count;
            return;
        }

        if (pos.m_node == m_head)
        {
            lastNode->setNext(m_head);
            m_head->setPrev(lastNode);
            m_head = firstNode;
            m_size += count;
            return;
        }

        T *curr = pos.m_node;
        T *prev = curr->getPrev();

        prev->setNext(firstNode);
        firstNode->setPrev(prev);
        lastNode->setNext(curr);
        curr->setPrev(lastNode);
        m_size += count;
    }

    /**
     * Slices and transfers range [first, last) from other list into this list before position pos.
     */
    void splice(iterator pos, IntrusiveLinkedList &other, iterator first, iterator last)
    {
        if (first == last || !first.m_node)
            return;

        size_t count = 0;
        for (auto it = first; it != last; ++it)
            ++count;

        splice(pos, other, first, last, count);
    }

    /**
     * Unlinks all nodes from the list and clears internal head, tail, and size states.
     */
    void clear()
    {
        T *curr = m_head;
        while (curr)
        {
            T *next = curr->getNext();
            curr->setPrev(nullptr);
            curr->setNext(nullptr);
            curr = next;
        }
        m_head = nullptr;
        m_tail = nullptr;
        m_size = 0;
    }

    /**
     * Converts a raw node pointer to an iterator bound to this list.
     */
    iterator to_iterator(T *node) { return iterator(node, this); }

    /**
     * Converts a const raw node pointer to a const iterator bound to this list.
     */
    const_iterator to_iterator(const T *node) const { return const_iterator(const_cast<T *>(node), this); }

  private:
    T *m_head{ nullptr }; // First node in the list, or nullptr when empty.
    T *m_tail{ nullptr }; // Last node in the list, or nullptr when empty.
    size_t m_size{ 0 };   // Cached element count kept in sync by link/unlink operations.
};

#endif // EZCORE_INTRUSIVE_LIST_H