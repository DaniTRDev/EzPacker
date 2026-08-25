#ifndef EZCORE_INTRUSIVE_LIST_H
#define EZCORE_INTRUSIVE_LIST_H

#include <cstddef>
#include <iterator>

/**
 * This class is here to replaced std::pmr::list with a linked list of already-constructed nodes. Instead of making an
 * extra 24 bytes per inserted element, this makes each list owner use extra 24 bytes ONCE to hold the list. Or just 16
 * bytes to hold prev and next so this object can be constructed.
 */
template <typename T> class IntrusiveLinkedList
{
  public:
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

        iterator &operator++()
        {
            if (m_node)
                m_node = m_node->getNext();
            return *this;
        }

        iterator operator++(int)
        {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        iterator &operator--()
        {
            if (m_node)
                m_node = m_node->getPrev();
            else if (m_list)
                m_node = m_list->m_tail;
            return *this;
        }

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

    IntrusiveLinkedList(IntrusiveLinkedList &&other) noexcept :
        m_head(other.m_head), m_tail(other.m_tail), m_size(other.m_size)
    {
        other.m_head = nullptr;
        other.m_tail = nullptr;
        other.m_size = 0;
    }

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

    iterator begin() { return iterator(m_head, this); }
    iterator end() { return iterator(nullptr, this); }
    const_iterator begin() const { return const_iterator(m_head, this); }
    const_iterator end() const { return const_iterator(nullptr, this); }
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }

    reverse_iterator rbegin() { return reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

    bool empty() const { return m_size == 0; }
    size_t size() const { return m_size; }

    T *front() const { return m_head; }
    T *back() const { return m_tail; }

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

    void remove(T *node)
    {
        if (!node)
            return;
        erase(to_iterator(node));
    }

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

    iterator to_iterator(T *node) { return iterator(node, this); }
    const_iterator to_iterator(const T *node) const { return const_iterator(const_cast<T *>(node), this); }

  private:
    T *m_head{ nullptr };
    T *m_tail{ nullptr };
    size_t m_size{ 0 };
};

#endif // EZCORE_INTRUSIVE_LIST_H