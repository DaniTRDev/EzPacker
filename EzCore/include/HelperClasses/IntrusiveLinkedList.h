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
        T *m_node{ nullptr };
        const IntrusiveLinkedList *m_list{ nullptr };

      public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T *;
        using difference_type = std::ptrdiff_t;
        using pointer = T **;
        using reference = T *&;

        iterator() = default;
        iterator(T *node, const IntrusiveLinkedList *list) : m_node(node), m_list(list) {}

        T *operator*() const { return m_node; }
        T *operator->() const { return m_node; }
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

        bool operator==(const iterator &other) const { return m_node == other.m_node; }
        bool operator!=(const iterator &other) const { return m_node != other.m_node; }
    };

    using const_iterator = iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    IntrusiveLinkedList() = default;
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

    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }

    reverse_iterator rbegin() { return reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
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
    T *m_head{ nullptr };
    T *m_tail{ nullptr };
    size_t m_size{ 0 };
};

#endif // EZCORE_INTRUSIVE_LIST_H