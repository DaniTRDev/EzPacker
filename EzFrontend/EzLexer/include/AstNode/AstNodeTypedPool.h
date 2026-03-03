#ifndef EZPACKER_ASTNODETYPEDPOOL_H
#define EZPACKER_ASTNODETYPEDPOOL_H

#include "EzLexerCommon.h"
#include "AstNode.h"

/**
 * A specialized TypedPool for allocating AstNode-derived objects. Ensures proper memory alignment
 * and placement-new construction. Only trivially destructible types derived from AstNode may be
 * created through createNode(). Objects allocated here live until the pool itself is destroyed.
 */
class AstNodeTypedPool : public TypedPool
{
  public:
    /**
     * Creates a node in the pool. Returns a pointer to it if succeeded.
     * @tparam ElemType
     * @tparam Args
     * @param args
     * @return ElemType
     */
    template <typename ElemType, typename... Args>
        requires(std::is_trivially_destructible<ElemType>::value && std::is_base_of<AstNode, ElemType>::value)
    ElemType *createNode(Args &&...args)
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

        new (reservedPointer) ElemType(std::forward<Args>(args)...);

        // CRITICAL: Add padding to used size to maintain alignment for next alloc
        chunk->m_usedSize += (size + padding);

        return reservedPointer;
    }
};

#endif // EZPACKER_ASTNODETYPEDPOOL_H
