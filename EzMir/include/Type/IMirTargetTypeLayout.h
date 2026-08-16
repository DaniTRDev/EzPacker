#ifndef EZMIR_IMIR_TARGET_TYPE_LAYOUT_H
#define EZMIR_IMIR_TARGET_TYPE_LAYOUT_H

#include "EzMirCommon.h"

/**
 * This interface exists just to make EzMir be able to calculate type offsets correctly, depending on a target
 * architecture, without needing to depend on EzTriple at all.
 *
 * The methods defined hered are only used for PRIMITIVES (i8, i16, i64, ...) not for arrays or classes.
 */
class IMirTargetTypeLayout
{
  public:
    virtual ~IMirTargetTypeLayout() = default;

    /**
     * Returns the size IN BYTES of a pointer.
     */
    virtual size_t getPointerSizeInBytes() const = 0;

    /**
     * Returns the alignment needed for a given type, IN BYTES.
     */
    virtual size_t getTypeAlignmentInBytes(const class MirType *type) const = 0;

    /**
     * Returns the type size, IN BYTES, (including alignment) of a given type.
     */
    virtual size_t getTypeSizeInBytes(const class MirType *type) const = 0;
};

#endif // EZMIR_IMIR_TARGET_TYPE_LAYOUT_H
