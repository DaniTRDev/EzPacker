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

// Internal node structure for the linked list.
template <typename ElemType> struct TypedPoolNode
{
    TypedPoolNode *m_next{ nullptr };
    ElemType *m_object{ nullptr };
};

// The handle representing a list of objects (Slice).
template <typename ElemType> struct TypedPoolSlice
{
    size_t m_numElems{ 0 };
    class TypedPool *m_owner{ nullptr };
    TypedPoolNode<ElemType> *m_head{ nullptr };
    TypedPoolNode<ElemType> *m_tail{ nullptr };

    struct Iterator
    {
        TypedPoolNode<ElemType> *m_curr;

        ElemType *operator*() const { return m_curr->m_object; }

        Iterator &operator++()
        {
            if (m_curr)
                m_curr = m_curr->m_next;
            return *this;
        }

        bool operator!=(const Iterator &other) const { return m_curr != other.m_curr; }
    };

    Iterator begin() const { return Iterator{ m_head }; }
    Iterator end() const { return Iterator{ nullptr }; } // nullptr marks the end of the list

    template <typename ElementType> ElementType *get(size_t index)
    {
        if (index >= m_numElems)
            return nullptr;

        size_t currentId = 0;
        TypedPoolNode<ElemType> *current = m_head;

        while (current && (currentId != index))
        {
            current = current->m_next;
            currentId++;
        }

        if (current)
        {
            // Ensure inheritance works.
            return dynamic_cast<ElementType *>(current->m_object);
        }

        return nullptr;
    }
};

/**
 * Allocates a pool of objects using a linear arena allocator.
 *
 * TODO: Move inlined template<typename ElementType> into the class. This will make working with slices and other
 * data structures a bit more intuitive, as the user won't have to specify the type of the slice and the element
 * separately. The type of the slice can be inferred from the type of the element being added to it. This will also help
 * with virtual objects and inheritance, as the slice can hold elements of an upper-in-inheritance class without needing
 * explicit casts.
 *
 * This will also allow to follow C++ STL guidelines and expose the slice type directly within the object / class
 * definition: for example, if we have `TypedPool<Type>` we can use TypedPool<Type>::Slice as the type of the slice, and
 * the user won't have to specify the type of the slice separately.
 */
class TypedPool
{
  public:
    /**
     * Ensure default constructor.
     */
    TypedPool() = default;

    /**
     * We don't need a destructor to manually free resources because we are using smart pointers (std::unique_ptr) to
     * manage memory. When the TypedPool instance is destroyed, the destructor of std::unique_ptr is called by the
     * destructor of the chunk list. It will automatically free the allocated memory for each chunk in m_chunks. This
     * approach simplifies memory management and helps prevent memory leaks.
     */

    // Disable copying to prevent double-free logic or pointer invalidation
    TypedPool(const TypedPool &) = delete;
    TypedPool &operator=(const TypedPool &) = delete;

    /**
     * Appends an existing element into a slice.
     */
    template <typename ElemType, typename SliceElemType>
    ElemType *appendToSlice(TypedPoolSlice<SliceElemType> *slice, ElemType *elem)
    {
        if (!slice || !elem)
            return nullptr;

        // Allocate a small node wrapper in the pool to hold the pointer
        TypedPoolNode<SliceElemType> *node = create<TypedPoolNode<SliceElemType>>();
        node->m_next = nullptr;
        node->m_object =
                reinterpret_cast<SliceElemType *>(elem); /*
                                                          * This cast is needed to resolve a problem in virtual
                                                          * objects. Having the slice be able to take any type, we
                                                          * make a slice of virtual objects (inheritance) be able to
                                                          * hold elements of an upper-in-inheritance class.
                                                          */

        if (slice->m_head == nullptr)
        {
            // First element in the list
            slice->m_head = node;
            slice->m_tail = node;
        }
        else
        {
            // Append to tail O(1)
            slice->m_tail->m_next = node;
            slice->m_tail = node;
        }

        slice->m_numElems++;
        return elem;
    }

    /**
     * Appends an element to a slice and inserts it in the front.
     * @tparam ElemType
     * @param slice
     * @param elem
     * @return ElemType *
     */
    template <typename ElemType, typename SliceElemType>
    ElemType *appendToSliceInFront(TypedPoolSlice<SliceElemType> *slice, ElemType *elem)
    {
        if (!slice || !elem)
            return nullptr;

        // Allocate a small node wrapper in the pool to hold the pointer
        TypedPoolNode<SliceElemType> *node = create<TypedPoolNode<SliceElemType>>();
        node->m_next = nullptr;
        node->m_object =
                reinterpret_cast<SliceElemType *>(elem); /*
                                                          * This cast is needed to resolve a problem in virtual
                                                          * objects. Having the slice be able to take any type, we
                                                          * make a slice of virtual objects (inheritance) be able to
                                                          * hold elements of an upper-in-inheritance class.
                                                          */

        if (slice->m_head == nullptr)
        {
            // First element in the list
            slice->m_head = node;
            slice->m_tail = node;
        }
        else
        {
            // Append to head O(1)
            node->m_next = slice->m_head;
            slice->m_head = node;
        }

        slice->m_numElems++;
        return elem;
    }

    /**
     * Allocates a new element in the pool.
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

        // Get current address to calculate strict alignment
        PoolChunk *chunk = &m_chunks.back();
        uintptr_t baseAddr = reinterpret_cast<uintptr_t>(chunk->m_data.get());
        uintptr_t currentAddr = baseAddr + chunk->m_usedSize;

        // Calculate padding required to align 'currentAddr'
        size_t padding = (alignment - (currentAddr % alignment)) % alignment;
        size_t totalNeeded = size + padding;

        // Check if fits in current chunk
        if (chunk->m_usedSize + totalNeeded > chunk->m_size)
        {
            allocateNewChunk(totalNeeded);
            chunk = &m_chunks.back();
            // New chunk starts at offset 0, so padding is 0 (assuming malloc aligns to max_align_t)
            padding = 0;
        }

        // Perform allocation
        uint8_t *startPtr = chunk->m_data.get();
        uint8_t *alignedPtr = startPtr + chunk->m_usedSize + padding;

        ElemType *reservedPointer = reinterpret_cast<ElemType *>(alignedPtr);

        chunk->m_usedSize += (size + padding);
        new (reservedPointer) ElemType(std::forward<Args>(args)...);

        return reservedPointer;
    }

    /**
     * Creates an element in the pool and pushes it to the given slice.
     */
    template <typename ElemType, typename SliceElemType, typename... Args>
        requires(std::is_trivially_destructible_v<ElemType>)
    ElemType *createAndAppendToSlice(TypedPoolSlice<SliceElemType> *slice, Args &&...args)
    {
        if (!slice)
            return nullptr;

        ElemType *elem = create<ElemType>(std::forward<Args>(args)...);
        return appendToSlice<ElemType, SliceElemType>(slice, elem);
    }

    /**
     * Creates an element in the pool and pushes it to the given slice.
     */
    template <typename ElemType, typename SliceElemType, typename... Args>
        requires(std::is_trivially_destructible_v<ElemType>)
    ElemType *createAndAppendToSliceInFront(TypedPoolSlice<SliceElemType> *slice, Args &&...args)
    {
        if (!slice)
            return nullptr;

        ElemType *elem = create<ElemType>(std::forward<Args>(args)...);
        return appendToSliceInFront<ElemType, SliceElemType>(slice, elem);
    }
    /**
     * Creates a slice (dynamic array handle) in the pool.
     */
    template <typename ElemType> TypedPoolSlice<ElemType> *createSlice()
    {
        // Allocate the handle itself in the pool
        return create<TypedPoolSlice<ElemType>>(0, this, nullptr, nullptr);
    }

    /**
     * Deallocates reserved memory.
     */
    void deallocate() { m_chunks.clear(); }

  protected:
    void allocateNewChunk(size_t size)
    {
        // This max ensures that if an object is larger than chunk size, it can still be allocated using a bigger chunk.
        size_t max = std::max(size, ChunkBlockSize);
        m_chunks.push_back(PoolChunk{ .m_size = max, .m_usedSize = 0, .m_data = std::make_unique<uint8_t[]>(max) });
    }

  protected:
    std::list<PoolChunk> m_chunks;
};

#endif // EZPACKER_TYPEDPOOL_H