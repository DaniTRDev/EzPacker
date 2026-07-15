#ifndef EZPACKER_IMIRTARGETTYPELAYOUT_H
#define EZPACKER_IMIRTARGETTYPELAYOUT_H

#include "EzMirCommon.h"
#include "MirType.h"

/**
 * This interface exists just to make EzMir be able to calculate type offsets correctly, depending on a target
 * architecture, without needing to depend on EzTriple at all.
 */
class IMirTargetTypeLayout
{
  public:
    virtual ~IMirTargetTypeLayout() = default;

    /**
     * Returns the size IN BYTES of a pointer.
     * @return
     */
    virtual size_t getPointerSizeInBytes() const = 0;

    /**
     * Returns the alignment needed for a given type, IN BYTES.
     * @param type
     * @return
     */
    virtual size_t getTypeAlignmentInBytes(const MirType *type) const = 0;

    /**
     * Returns the type size, IN BYTES, (including alignment) of a given type.
     * @param type
     * @return
     */
    virtual size_t getTypeSizeInBytes(const MirType *type) const = 0;
};

#endif // EZPACKER_IMIRTARGETTYPELAYOUT_H
