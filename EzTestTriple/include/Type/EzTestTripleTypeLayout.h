#ifndef EZPACKER_EZTESTTRIPLETYPELAYOUT_H
#define EZPACKER_EZTESTTRIPLETYPELAYOUT_H

#include "EzTestTripleCommon.h"

/**
 * This class defines the type layout for the test target.
 *
 * Pointer size: 64-bit (8 bytes), aligned to 8 bytes.
 * Type alignment:
 *  (SIZE IN BITS, ALIGNMENT IN BYTES)
 *    (<=8              , 1)
 *    (>8   AND <= 16   , 2)
 *    (>16  AND <= 32   , 4)
 *    (>32  AND <= 64   , 8)
 *    (>64              , 16)
 *    (Class            , the maximum alignment of its fields)
 *    (Array            , alignment of the internal type)
 *    (Any other case   , 1)
 *
 * Type size:
 *    (Void             , 0)
 *    (Int AND Float    , Nearest round-up byte (ALIGNED))
 *    (Pointer          , 8)
 *    (Array            , elemCount * elemSize (ALIGNED))
 *    (Class            , total ALIGNED size to contain VTABLE and FIELDS)
 */
class EzTestTripleTypeLayout : public IMirTargetTypeLayout
{
  public:
    size_t getPointerSizeInBytes() const override;

    size_t getTypeAlignmentInBytes(const MirType *type) const override;

    size_t getTypeSizeInBytes(const MirType *type) const override;
};

#endif // EZPACKER_EZTESTTRIPLETYPELAYOUT_H