#ifndef EZPACKER_TESTTARGETTYPELAYOUT_H
#define EZPACKER_TESTTARGETTYPELAYOUT_H

#include "Type/IMirTargetTypeLayout.h"

/**
 * This class represents a test target layout based on x64.
 */
class TestTargetTypeLayout : public IMirTargetTypeLayout
{
  public:
    /**
     * x64 pointers are always 8 bytes (64-bit).
     */
    size_t getPointerSizeInBytes() const override;

    /**
     * Returns the x64 natural alignment for the given type.
     */
    size_t getTypeAlignmentInBytes(const MirType *type) const override;

    /**
     * Returns the size of the given type, padded to fit alignment parameters.
     */
    size_t getTypeSizeInBytes(const MirType *type) const override;
};

#endif // EZPACKER_TESTTARGETTYPELAYOUT_H