#ifndef EZPACKER_TYPEDPOOL_H
#define EZPACKER_TYPEDPOOL_H

#include "EzCoreCommon.h"
#include <list>
#include <memory>
#include <cstdint>
#include <type_traits>

constexpr size_t ChunkBlockSize = 1024 * 16; // 16KB block size.

struct PoolChunk
{
    size_t m_sizeInBytes{ 0 }; // In bytes.
    size_t m_usedSize{ 0 };    // In bytes.
    std::unique_ptr<uint8_t[]> m_data;
};

// Internal node structure for the doubly-linked list.
template <typename ElemType> struct TypedPoolLinkedListNode
{
    TypedPoolLinkedListNode *m_next{ nullptr };
    TypedPoolLinkedListNode *m_prev{ nullptr }; // <-- Added for backwards iteration
    ElemType *m_object{ nullptr };
};

// The handle representing a list of objects (Slice).
template <typename ElemType> struct TypedPoolLinkedList
{
    size_t m_numElems{ 0 };
    class TypedPool *m_owner{ nullptr };
    TypedPoolLinkedListNode<ElemType> *m_head{ nullptr };
    TypedPoolLinkedListNode<ElemType> *m_tail{ nullptr };

    // Standard Forward Iterator (Bidirectional)
    struct Iterator
    {
        TypedPoolLinkedListNode<ElemType> *m_curr{ nullptr };

        explicit operator bool() const { return m_curr != nullptr; }

        bool operator!() { return m_curr == nullptr; }

        ElemType *operator*() const { return m_curr->m_object; }

        Iterator &operator++()
        {
            if (m_curr)
                m_curr = m_curr->m_next;
            return *this;
        }

        Iterator &operator--()
        {
            if (m_curr)
                m_curr = m_curr->m_prev;
            
            return *this;
        }

        Iterator &operator+(size_t id)
        {
            size_t it = 0;
            while (it < id)
            {
                if (!m_curr)
                {
                    break;
                }

                m_curr = m_curr->m_next;
                it++;
            }

            return *this;
        }

        bool operator!=(const Iterator &other) const { return m_curr != other.m_curr; }

        bool operator==(const Iterator &other) const { return m_curr == other.m_curr; }

        /**
         * @brief Swaps the item of the list this iterator points to.
         * @param other The other iterator to swap with.
         */
        void swapItem(const Iterator &other) { m_curr->m_object = other.m_curr->m_object; }
    };

    // Reverse Iterator
    struct ReverseIterator
    {
        TypedPoolLinkedListNode<ElemType> *m_curr{ nullptr };

        explicit operator bool() const { return m_curr != nullptr; }

        bool operator!() { return m_curr == nullptr; }

        ElemType *operator*() const { return m_curr->m_object; }

        // Incrementing a reverse iterator moves BACKWARDS in the list
        ReverseIterator &operator++()
        {
            if (m_curr)
                m_curr = m_curr->m_prev;
            return *this;
        }

        // Decrementing a reverse iterator moves FORWARDS in the list
        ReverseIterator &operator--()
        {
            if (m_curr)
                m_curr = m_curr->m_next;
            
            
            return *this;
        }

        bool operator!=(const ReverseIterator &other) const { return m_curr != other.m_curr; }

        bool operator!=(const Iterator &other) const { return m_curr != other.m_curr; }
    };

    Iterator begin() const { return Iterator{ m_head }; }
    Iterator end() const { return Iterator{ nullptr }; } // nullptr marks the end of the list

    ReverseIterator rbegin() const { return ReverseIterator{ m_tail }; }
    ReverseIterator rend() const { return ReverseIterator{ nullptr }; }

    /**
     * @brief Retrieves an element from the list by its index.
     * @param index The index of the element to retrieve.
     * @returns A pointer to the element, or nullptr if the index is out of bounds.
     */
    template <typename ElementType> ElementType *get(size_t index) const
    {
        if (index >= m_numElems)
            return nullptr;

        size_t currentId = 0;
        TypedPoolLinkedListNode<ElemType> *current = m_head;

        // Optimization: If index is in the second half, iterate backwards
        if (index > m_numElems / 2)
        {
            currentId = m_numElems - 1;
            current = m_tail;
            while (current && (currentId != index))
            {
                current = current->m_prev;
                currentId--;
            }
        }
        else
        {
            while (current && (currentId != index))
            {
                current = current->m_next;
                currentId++;
            }
        }

        if (current)
        {
            // Ensure inheritance works.
            return dynamic_cast<ElementType *>(current->m_object);
        }

        return nullptr;
    }

    /**
     * @brief Iterates over the linked list and applies a callback to each element.
     * @param begin The starting iterator.
     * @param end The ending iterator.
     * @param callback The function to apply to each element.
     */
    void forEach(TypedPoolLinkedList<ElemType>::Iterator begin,
                 TypedPoolLinkedList<ElemType>::Iterator end,
                 const std::function<bool(ElemType *elem, size_t pos)> &callback) const
    {
        size_t index = 0;
        Iterator current = begin;

        while (current != end)
        {
            if (!callback(*current, index))
                break;

            ++current;
            index++;
        }
    }

    /**
     * @brief Iterates over the linked list and applies a callback to each iterator.
     * @param begin The starting iterator.
     * @param end The ending iterator.
     * @param callback The function to apply to each iterator.
     */
    void forEach(TypedPoolLinkedList<ElemType>::Iterator begin,
                 TypedPoolLinkedList<ElemType>::Iterator end,
                 const std::function<bool(TypedPoolLinkedList<ElemType> *list,
                                          TypedPoolLinkedList<ElemType>::Iterator it)> &callback) const
    {
        Iterator current = begin;

        while (current != end)
        {
            if (!callback(this, current))
                break;

            ++current;
        }
    }
};

class TypedPool
{
  public:
    TypedPool() = default;

    TypedPool(const TypedPool &) = delete;
    TypedPool &operator=(const TypedPool &) = delete;

    /**
     * @brief Appends an existing element to the back of a linked list.
     * @param list The list to append to.
     * @param elem The element to append.
     * @returns The appended element, or nullptr on failure.
     */
    template <typename ElemType, typename LinkedListType>
    ElemType *appendToListBack(TypedPoolLinkedList<LinkedListType> *list, ElemType *elem)
    {
        if (!list || !elem)
            return nullptr;

        TypedPoolLinkedListNode<LinkedListType> *node = create<TypedPoolLinkedListNode<LinkedListType>>();
        node->m_next = nullptr;
        node->m_object = reinterpret_cast<LinkedListType *>(elem);

        if (list->m_head == nullptr)
        {
            node->m_prev = nullptr;
            list->m_head = node;
            list->m_tail = node;
        }
        else
        {
            node->m_prev = list->m_tail;
            list->m_tail->m_next = node;
            list->m_tail = node;
        }

        list->m_numElems++;
        return elem;
    }

    /**
     * @brief Appends an existing element to the front of a linked list.
     * @param list The list to append to.
     * @param elem The element to append.
     * @returns The appended element, or nullptr on failure.
     */
    template <typename ElemType, typename LinkedListType>
    ElemType *appendToListFront(TypedPoolLinkedList<LinkedListType> *list, ElemType *elem)
    {
        if (!list || !elem)
            return nullptr;

        TypedPoolLinkedListNode<LinkedListType> *node = create<TypedPoolLinkedListNode<LinkedListType>>();
        node->m_prev = nullptr;
        node->m_object = reinterpret_cast<LinkedListType *>(elem);

        if (list->m_head == nullptr)
        {
            node->m_next = nullptr;
            list->m_head = node;
            list->m_tail = node;
        }
        else
        {
            node->m_next = list->m_head;
            list->m_head->m_prev = node;
            list->m_head = node;
        }

        list->m_numElems++;
        return elem;
    }

    /**
     * @brief Inserts an element into a list after the node pointed to by the iterator.
     * @param list The list to insert into.
     * @param it The iterator pointing to the node after which to insert.
     * @param elem The element to insert.
     * @returns The inserted element, or nullptr on failure.
     */
    template <typename ElemType, typename LinkedListType>
    ElemType *appendToListAfter(TypedPoolLinkedList<LinkedListType> *list,
                                typename TypedPoolLinkedList<LinkedListType>::Iterator it,
                                ElemType *elem)
    {
        if (!list || !elem || !it.m_curr)
            return nullptr;

        TypedPoolLinkedListNode<LinkedListType> *node = create<TypedPoolLinkedListNode<LinkedListType>>();
        node->m_object = reinterpret_cast<LinkedListType *>(elem);

        TypedPoolLinkedListNode<LinkedListType> *targetNode = it.m_curr;

        // Wire up the new node
        node->m_prev = targetNode;
        node->m_next = targetNode->m_next;

        // Wire up the surrounding nodes
        if (targetNode->m_next)
        {
            targetNode->m_next->m_prev = node;
        }
        else
        {
            // If we inserted after the tail, we are the new tail
            list->m_tail = node;
        }

        targetNode->m_next = node;
        list->m_numElems++;
        ++it;

        return elem;
    }

    /**
     * @brief Inserts an element into a list before the node pointed to by the iterator.
     * @param list The list to insert into.
     * @param it The iterator pointing to the node before which to insert.
     * @param elem The element to insert.
     * @returns The inserted element, or nullptr on failure.
     */
    template <typename ElemType, typename LinkedListType>
    ElemType *appendToListBefore(TypedPoolLinkedList<LinkedListType> *list,
                                 typename TypedPoolLinkedList<LinkedListType>::Iterator it,
                                 ElemType *elem)
    {
        if (!list || !elem)
            return nullptr;

        // If the iterator is at the end (m_curr == nullptr), inserting BEFORE the end is the exact same as appending to
        // the BACK of the list.
        if (!it.m_curr)
        {
            return appendToListBack<ElemType, LinkedListType>(list, elem);
        }

        TypedPoolLinkedListNode<LinkedListType> *node = create<TypedPoolLinkedListNode<LinkedListType>>();
        node->m_object = reinterpret_cast<LinkedListType *>(elem);

        TypedPoolLinkedListNode<LinkedListType> *targetNode = it.m_curr;

        // Wire up the new node
        node->m_next = targetNode;
        node->m_prev = targetNode->m_prev;

        // Wire up the surrounding nodes
        if (targetNode->m_prev)
        {
            targetNode->m_prev->m_next = node;
        }
        else
        {
            // If we inserted before the head, we are the new head
            list->m_head = node;
        }

        targetNode->m_prev = node;
        list->m_numElems++;

        return elem;
    }

    /**
     * @brief Creates a new object in the pool.
     * @param args Arguments to forward to the object's constructor.
     * @returns A pointer to the newly created object.
     */
    template <typename ElemType, typename... Args>
        requires(std::is_trivially_destructible_v<ElemType>)
    ElemType *create(Args &&...args)
    {
        size_t size = sizeof(ElemType);
        size_t alignment = alignof(ElemType);

        if (m_chunks.empty())
        {
            allocateNewChunk(size);
        }

        PoolChunk *chunk = &m_chunks.back();
        uintptr_t baseAddr = reinterpret_cast<uintptr_t>(chunk->m_data.get());
        uintptr_t currentAddr = baseAddr + chunk->m_usedSize;

        size_t padding = (alignment - (currentAddr % alignment)) % alignment;
        size_t totalNeeded = size + padding;

        if (chunk->m_usedSize + totalNeeded > chunk->m_sizeInBytes)
        {
            allocateNewChunk(totalNeeded);
            chunk = &m_chunks.back();
            padding = 0;
        }

        uint8_t *startPtr = chunk->m_data.get();
        uint8_t *alignedPtr = startPtr + chunk->m_usedSize + padding;

        ElemType *reservedPointer = reinterpret_cast<ElemType *>(alignedPtr);

        chunk->m_usedSize += (size + padding);
        new (reservedPointer) ElemType(std::forward<Args>(args)...);

        return reservedPointer;
    }

    /**
     * @brief Creates a new object and appends it to the back of a linked list.
     * @param list The list to append to.
     * @param args Arguments to forward to the object's constructor.
     * @returns A pointer to the newly created object, or nullptr on failure.
     */
    template <typename ElemType, typename LinkedListType, typename... Args>
        requires(std::is_trivially_destructible_v<ElemType>)
    ElemType *createAndAppendToListBack(TypedPoolLinkedList<LinkedListType> *list, Args &&...args)
    {
        if (!list)
            return nullptr;

        ElemType *elem = create<ElemType>(std::forward<Args>(args)...);
        return appendToListBack<ElemType, LinkedListType>(list, elem);
    }

    /**
     * @brief Creates a new object and appends it to the front of a linked list.
     * @param list The list to append to.
     * @param args Arguments to forward to the object's constructor.
     * @returns A pointer to the newly created object, or nullptr on failure.
     */
    template <typename ElemType, typename LinkedListType, typename... Args>
        requires(std::is_trivially_destructible_v<ElemType>)
    ElemType *createAndAppendToListFront(TypedPoolLinkedList<LinkedListType> *list, Args &&...args)
    {
        if (!list)
            return nullptr;

        ElemType *elem = create<ElemType>(std::forward<Args>(args)...);
        return appendToListFront<ElemType, LinkedListType>(list, elem);
    }

    /**
     * @brief Creates a new object and inserts it after the specified iterator.
     * @param list The list to insert into.
     * @param it The iterator to insert after.
     * @param args Arguments to forward to the object's constructor.
     * @returns A pointer to the newly created object, or nullptr on failure.
     */
    template <typename ElemType, typename LinkedListType, typename... Args>
        requires(std::is_trivially_destructible_v<ElemType>)
    ElemType *createAndAppendToListAfter(TypedPoolLinkedList<LinkedListType> *list,
                                         typename TypedPoolLinkedList<LinkedListType>::Iterator it,
                                         Args &&...args)
    {
        if (!list)
            return nullptr;

        ElemType *elem = create<ElemType>(std::forward<Args>(args)...);
        return appendToListAfter<ElemType, LinkedListType>(list, it, elem);
    }

    /**
     * @brief Creates a new object and inserts it before the specified iterator.
     * @param list The list to insert into.
     * @param it The iterator to insert before.
     * @param args Arguments to forward to the object's constructor.
     * @returns A pointer to the newly created object, or nullptr on failure.
     */
    template <typename ElemType, typename LinkedListType, typename... Args>
        requires(std::is_trivially_destructible_v<ElemType>)
    ElemType *createAndAppendToListBefore(TypedPoolLinkedList<LinkedListType> *list,
                                          typename TypedPoolLinkedList<LinkedListType>::Iterator it,
                                          Args &&...args)
    {
        if (!list)
            return nullptr;

        ElemType *elem = create<ElemType>(std::forward<Args>(args)...);
        return appendToListBefore<ElemType, LinkedListType>(list, it, elem);
    }

    /**
     * @brief Removes the element pointed to by the iterator from the list.
     * @param list The list to remove from.
     * @param it The iterator pointing to the element to remove.
     * @returns An iterator to the next valid element in the list.
     */
    template <typename LinkedListType>
    typename TypedPoolLinkedList<LinkedListType>::Iterator
    removeFromList(TypedPoolLinkedList<LinkedListType> *list, typename TypedPoolLinkedList<LinkedListType>::Iterator it)
    {
        if (!list || !it.m_curr)
            return typename TypedPoolLinkedList<LinkedListType>::Iterator{ nullptr };

        TypedPoolLinkedListNode<LinkedListType> *targetNode = it.m_curr;
        TypedPoolLinkedListNode<LinkedListType> *nextNode = targetNode->m_next;
        TypedPoolLinkedListNode<LinkedListType> *prevNode = targetNode->m_prev;

        // Wire up the previous node (or update the head if we are removing the first element)
        if (prevNode)
        {
            prevNode->m_next = nextNode;
        }
        else
        {
            list->m_head = nextNode;
        }

        // Wire up the next node (or update the tail if we are removing the last element)
        if (nextNode)
        {
            nextNode->m_prev = prevNode;
        }
        else
        {
            list->m_tail = prevNode;
        }

        list->m_numElems--;

        // Isolate the removed node to prevent accidental traversal bugs
        targetNode->m_next = nullptr;
        targetNode->m_prev = nullptr;

        // Return an iterator pointing to the NEXT element
        return typename TypedPoolLinkedList<LinkedListType>::Iterator{ nextNode };
    }

    /**
     * @brief Creates a new, empty linked list in the pool.
     * @returns A pointer to the newly created linked list.
     */
    template <typename ElemType> TypedPoolLinkedList<ElemType> *createLinkedList()
    {
        return create<TypedPoolLinkedList<ElemType>>(0, this, nullptr, nullptr);
    }

    /**
     * @brief Deallocates all memory used by the pool.
     * @returns void
     */
    void deallocate() { m_chunks.clear(); }

  protected:
    void allocateNewChunk(size_t size)
    {
        size_t max = std::max(size, ChunkBlockSize);
        m_chunks.push_back(
                PoolChunk{ .m_sizeInBytes = max, .m_usedSize = 0, .m_data = std::make_unique<uint8_t[]>(max) });
    }

  protected:
    std::list<PoolChunk> m_chunks;
};

#endif // EZPACKER_TYPEDPOOL_H
