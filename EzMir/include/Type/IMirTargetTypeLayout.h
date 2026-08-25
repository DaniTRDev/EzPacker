#ifndef EZMIR_IMIR_TARGET_TYPE_LAYOUT_H
#define EZMIR_IMIR_TARGET_TYPE_LAYOUT_H

#include "EzMirCommon.h"

/**
 * Interface enabling target-specific type layout calculations (sizes, alignments, pointer sizing)
 * without requiring EzMir to directly depend on EzTriple.
 *
 * Provides primitive data type queries (i8, i16, i32, i64, f32, f64, pointers) for memory operand sizing,
 * structure layouts, and calling convention lowering.
 */
class IMirTargetTypeLayout
{
  public:
    /**
     * Virtual destructor for interface cleanup.
     */
    virtual ~IMirTargetTypeLayout() = default;

    /**
     * Returns the size of a machine pointer in bytes for the target architecture.
     */
    virtual size_t getPointerSizeInBytes() const = 0;

    /**
     * Returns the required memory alignment in bytes for the specified primitive MIR type.
     */
    virtual size_t getTypeAlignmentInBytes(const class MirType *type) const = 0;

    /**
     * Returns the allocation size in bytes (including ABI padding/alignment) for the specified primitive MIR type.
     */
    virtual size_t getTypeSizeInBytes(const class MirType *type) const = 0;
};

#endif // EZMIR_IMIR_TARGET_TYPE_LAYOUT_H
