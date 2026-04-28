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
     * @param alignment
     * @param shadowSpace
     */
    StackLayout(size_t alignment, size_t shadowSpace);

    /**
     * Aligns the given address for this stack layout.
     * @param addr
     * @return size_t
     */
    size_t alignAddress(size_t addr) const;

    /**
     * @brief Returns the alignment of the stack layout.
     * @return size_t
     */
    size_t getAlignment() const;

    /**
     * @brief Returns the shadow space of the stack layout.
     * @return size_t
     */
    size_t getShadowSpace() const;

  private:
    size_t m_alignment;
    size_t m_shadowSpace;
};

#endif // EZPACKER_STACKLAYOUT_H
