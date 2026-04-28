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
    size_t m_size{ 0 };     // In bytes.
    size_t m_usedSize{ 0 }; // In bytes.
    std::unique_ptr<uint8_t[]> m_data;
};

// Internal node structure for the doubly-linked list.
template <typename ElemType> struct TypedPoolNode
{
    TypedPoolNode *m_next{ nullptr };
    TypedPoolNode *m_prev{ nullptr }; // <-- Added for backwards iteration
    ElemType *m_object{ nullptr };
};

// The handle representing a list of objects (Slice).
template <typename ElemType> struct TypedPoolSlice
{
    size_t m_numElems{ 0 };
    class TypedPool *m_owner{ nullptr };
    TypedPoolNode<ElemType> *m_head{ nullptr };
    TypedPoolNode<ElemType> *m_tail{ nullptr };

    // Standard Forward Iterator (Bidirectional)
    struct Iterator
    {
        TypedPoolNode<ElemType> *m_curr{ nullptr };

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
    };

    // Reverse Iterator
    struct ReverseIterator
    {
        TypedPoolNode<ElemType> *m_curr{ nullptr };

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
    };

    Iterator begin() const { return Iterator{ m_head }; }
    Iterator end() const { return Iterator{ nullptr }; } // nullptr marks the end of the list

    ReverseIterator rbegin() const { return ReverseIterator{ m_tail }; }
    ReverseIterator rend() const { return ReverseIterator{ nullptr }; }

    template <typename ElementType> ElementType *get(size_t index) const
    {
        if (index >= m_numElems)
            return nullptr;

        size_t currentId = 0;
        TypedPoolNode<ElemType> *current = m_head;

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
};

class TypedPool
{
  public:
    TypedPool() = default;

    TypedPool(const TypedPool &) = delete;
    TypedPool &operator=(const TypedPool &) = delete;

    template <typename ElemType, typename SliceElemType>
    ElemType *appendToSlice(TypedPoolSlice<SliceElemType> *slice, ElemType *elem)
    {
        if (!slice || !elem)
            return nullptr;

        TypedPoolNode<SliceElemType> *node = create<TypedPoolNode<SliceElemType>>();
        node->m_next = nullptr;
        node->m_object = reinterpret_cast<SliceElemType *>(elem);

        if (slice->m_head == nullptr)
        {
            node->m_prev = nullptr;
            slice->m_head = node;
            slice->m_tail = node;
        }
        else
        {
            node->m_prev = slice->m_tail;
            slice->m_tail->m_next = node;
            slice->m_tail = node;
        }

        slice->m_numElems++;
        return elem;
    }

    template <typename ElemType, typename SliceElemType>
    ElemType *appendToSliceInFront(TypedPoolSlice<SliceElemType> *slice, ElemType *elem)
    {
        if (!slice || !elem)
            return nullptr;

        TypedPoolNode<SliceElemType> *node = create<TypedPoolNode<SliceElemType>>();
        node->m_prev = nullptr;
        node->m_object = reinterpret_cast<SliceElemType *>(elem);

        if (slice->m_head == nullptr)
        {
            node->m_next = nullptr;
            slice->m_head = node;
            slice->m_tail = node;
        }
        else
        {
            node->m_next = slice->m_head;
            slice->m_head->m_prev = node;
            slice->m_head = node;
        }

        slice->m_numElems++;
        return elem;
    }

    /**
     * Appends an element into a slice immediately AFTER the node pointed to by the iterator.
     */
    template <typename ElemType, typename SliceElemType>
    ElemType *appendToSliceAfter(TypedPoolSlice<SliceElemType> *slice,
                                 typename TypedPoolSlice<SliceElemType>::Iterator it,
                                 ElemType *elem)
    {
        if (!slice || !elem || !it.m_curr)
            return nullptr;

        TypedPoolNode<SliceElemType> *node = create<TypedPoolNode<SliceElemType>>();
        node->m_object = reinterpret_cast<SliceElemType *>(elem);

        TypedPoolNode<SliceElemType> *targetNode = it.m_curr;

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
            slice->m_tail = node;
        }

        targetNode->m_next = node;
        slice->m_numElems++;

        return elem;
    }

    /**
     * Appends an element into a slice immediately BEFORE the node pointed to by the iterator.
     */
    template <typename ElemType, typename SliceElemType>
    ElemType *appendToSliceBefore(TypedPoolSlice<SliceElemType> *slice,
                                  typename TypedPoolSlice<SliceElemType>::Iterator it,
                                  ElemType *elem)
    {
        if (!slice || !elem || !it.m_curr)
            return nullptr;

        TypedPoolNode<SliceElemType> *node = create<TypedPoolNode<SliceElemType>>();
        node->m_object = reinterpret_cast<SliceElemType *>(elem);

        TypedPoolNode<SliceElemType> *targetNode = it.m_curr;

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
            slice->m_head = node;
        }

        targetNode->m_prev = node;
        slice->m_numElems++;

        return elem;
    }

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

        if (chunk->m_usedSize + totalNeeded > chunk->m_size)
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

    template <typename ElemType, typename SliceElemType, typename... Args>
        requires(std::is_trivially_destructible_v<ElemType>)
    ElemType *createAndAppendToSlice(TypedPoolSlice<SliceElemType> *slice, Args &&...args)
    {
        if (!slice)
            return nullptr;

        ElemType *elem = create<ElemType>(std::forward<Args>(args)...);
        return appendToSlice<ElemType, SliceElemType>(slice, elem);
    }

    template <typename ElemType, typename SliceElemType, typename... Args>
        requires(std::is_trivially_destructible_v<ElemType>)
    ElemType *createAndAppendToSliceInFront(TypedPoolSlice<SliceElemType> *slice, Args &&...args)
    {
        if (!slice)
            return nullptr;

        ElemType *elem = create<ElemType>(std::forward<Args>(args)...);
        return appendToSliceInFront<ElemType, SliceElemType>(slice, elem);
    }

    template <typename ElemType, typename SliceElemType, typename... Args>
        requires(std::is_trivially_destructible_v<ElemType>)
    ElemType *createAndAppendToSliceAfter(TypedPoolSlice<SliceElemType> *slice,
                                          typename TypedPoolSlice<SliceElemType>::Iterator it,
                                          Args &&...args)
    {
        if (!slice)
            return nullptr;

        ElemType *elem = create<ElemType>(std::forward<Args>(args)...);
        return appendToSliceAfter<ElemType, SliceElemType>(slice, it, elem);
    }

    template <typename ElemType, typename SliceElemType, typename... Args>
        requires(std::is_trivially_destructible_v<ElemType>)
    ElemType *createAndAppendToSliceBefore(TypedPoolSlice<SliceElemType> *slice,
                                           typename TypedPoolSlice<SliceElemType>::Iterator it,
                                           Args &&...args)
    {
        if (!slice)
            return nullptr;

        ElemType *elem = create<ElemType>(std::forward<Args>(args)...);
        return appendToSliceBefore<ElemType, SliceElemType>(slice, it, elem);
    }

    /**
     * Removes the element pointed to by the iterator from the slice.
     * Returns an iterator to the NEXT valid element, allowing safe removal during iteration.
     */
    template <typename SliceElemType>
    typename TypedPoolSlice<SliceElemType>::Iterator
    removeFromSlice(TypedPoolSlice<SliceElemType> *slice, typename TypedPoolSlice<SliceElemType>::Iterator it)
    {
        if (!slice || !it.m_curr)
            return typename TypedPoolSlice<SliceElemType>::Iterator{ nullptr };

        TypedPoolNode<SliceElemType> *targetNode = it.m_curr;
        TypedPoolNode<SliceElemType> *nextNode = targetNode->m_next;
        TypedPoolNode<SliceElemType> *prevNode = targetNode->m_prev;

        // Wire up the previous node (or update the head if we are removing the first element)
        if (prevNode)
        {
            prevNode->m_next = nextNode;
        }
        else
        {
            slice->m_head = nextNode;
        }

        // Wire up the next node (or update the tail if we are removing the last element)
        if (nextNode)
        {
            nextNode->m_prev = prevNode;
        }
        else
        {
            slice->m_tail = prevNode;
        }

        slice->m_numElems--;

        // Isolate the removed node to prevent accidental traversal bugs
        targetNode->m_next = nullptr;
        targetNode->m_prev = nullptr;

        // Return an iterator pointing to the NEXT element
        return typename TypedPoolSlice<SliceElemType>::Iterator{ nextNode };
    }

    template <typename ElemType> TypedPoolSlice<ElemType> *createSlice()
    {
        return create<TypedPoolSlice<ElemType>>(0, this, nullptr, nullptr);
    }

    void deallocate() { m_chunks.clear(); }

  protected:
    void allocateNewChunk(size_t size)
    {
        size_t max = std::max(size, ChunkBlockSize);
        m_chunks.push_back(PoolChunk{ .m_size = max, .m_usedSize = 0, .m_data = std::make_unique<uint8_t[]>(max) });
    }

  protected:
    std::list<PoolChunk> m_chunks;
};

#endif // EZPACKER_TYPEDPOOL_H