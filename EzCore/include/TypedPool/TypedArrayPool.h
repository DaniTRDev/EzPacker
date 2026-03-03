#ifndef EZPACKER_TYPEDARRAYPOOL_H
#define EZPACKER_TYPEDARRAYPOOL_H

#include "EzCoreCommon.h"
#include "TypedPool.h"

template <typename ElemType> struct ConstantArray
{
    ElemType *m_elems{ nullptr };
    size_t m_numElems{ 0 };

    struct Iterator
    {
        ElemType *m_curr;

        void *operator*() const { return *m_curr; }

        Iterator &operator++()
        {
            if (m_curr)
                m_curr++;

            return *this;
        }

        bool operator!=(const Iterator &other) const { return m_curr != other.m_curr; }
    };

    Iterator begin() const { return Iterator{ m_elems }; }
    Iterator end() const { return Iterator{ nullptr }; } // nullptr marks the end of the list
};

/**
 * This class acts as a fixed-size array container. ElemType MUST have a default constructor.
 * @tparam ElemType
 */
template <typename ElemType> class TypedArrayPool : public TypedPool
{
  public:
    /**
     * Creates a fixed-size array in the pool.
     * @param count
     * @return ElemType*
     */
    ConstantArray<ElemType> createConstantArray(size_t count)
    {
        if (count == 0)
            return { .m_elems = nullptr, .m_numElems = 0 };

        size_t size = sizeof(ElemType) * count; // Total size for array
        size_t alignment = alignof(ElemType);

        if (m_chunks.empty())
            allocateNewChunk(size);

        PoolChunk *chunk = &m_chunks.back();
        uintptr_t baseAddr = reinterpret_cast<uintptr_t>(chunk->m_data.get());
        uintptr_t currentAddr = baseAddr + chunk->m_usedSize;
        size_t padding = (alignment - (currentAddr % alignment)) % alignment;
        size_t totalNeeded = size + padding;

        // Check if fits
        if (chunk->m_usedSize + totalNeeded > chunk->m_size)
        {
            allocateNewChunk(totalNeeded);
            chunk = &m_chunks.back();
            padding = 0;
        }

        uint8_t *startPtr = chunk->m_data.get();
        uint8_t *alignedPtr = startPtr + chunk->m_usedSize + padding;

        // Update used size
        chunk->m_usedSize += totalNeeded;

        // We don't construct the elements here because we usually
        // copy into them immediately after.
        // But to be safe, we can default initialize:
        ElemType *ptr = reinterpret_cast<ElemType *>(alignedPtr);
        for (size_t i = 0; i < count; ++i)
            new (&ptr[i]) ElemType();

        return ConstantArray{ .m_elems = ptr, .m_numElems = count };
    }

    ConstantArray<ElemType> createConstantArray(const void *init, size_t count)
    {
        ConstantArray<ElemType> res = createConstantArray(count);
        std::copy_n((ElemType *)init, res.m_numElems, res.m_elems);

        return res;
    }
};

#endif // EZPACKER_TYPEDARRAYPOOL_H
