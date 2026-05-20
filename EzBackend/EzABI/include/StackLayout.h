#ifndef EZPACKER_STACKLAYOUT_H
#define EZPACKER_STACKLAYOUT_H

#include "EzABICommon.h"

/**
 * Made its own class and file so it is extensible for the future.
 */
class StackLayout
{
  public:
    /**
     * @brief Constructs a StackLayout with the given alignment and shadow space.
     * @param alignment The stack alignment in bytes.
     * @param shadowSpace The size of the shadow space in bytes.
     */
    StackLayout(size_t alignment, size_t shadowSpace);

    /**
     * @brief Aligns the given address for this stack layout.
     * @param addr The address to align.
     * @returns The aligned address.
     */
    int64_t alignAddress(int64_t addr) const;

    /**
     * @brief Returns the alignment of the stack layout.
     * @returns The stack alignment in bytes.
     */
    size_t getAlignment() const;

    /**
     * @brief Returns the shadow space of the stack layout.
     * @returns The shadow space size in bytes.
     */
    size_t getShadowSpace() const;

  private:
    size_t m_alignment;
    size_t m_shadowSpace;
};

#endif // EZPACKER_STACKLAYOUT_H
