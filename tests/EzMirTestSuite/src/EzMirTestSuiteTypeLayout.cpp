#include "EzMirTestSuiteTypeLayout.h"
#include "Type/MirType.h"

size_t EzMirTestSuiteTypeLayout::getPointerSizeInBytes() const { return 8; }

size_t EzMirTestSuiteTypeLayout::getTypeAlignmentInBytes(const MirType *type) const
{
    if (!type)
        return 1;

    switch (type->getKind())
    {
        case MirTypeKind::Void:
            return 1;

        case MirTypeKind::Integer:
        case MirTypeKind::FloatingPoint:
        {
            size_t sizeInBits = type->getTotalSizeInBits();
            if (sizeInBits <= 8)
                return 1; // i1, i8
            if (sizeInBits <= 16)
                return 2; // i16
            if (sizeInBits <= 32)
                return 4; // i32, f32
            if (sizeInBits <= 64)
                return 8; // i64, f64
            if (sizeInBits <= 128)
                return 16; // i128, f128 (aligned to 16 bytes on SSE/AVX boundary)
            return 32;     // i256, etc. (aligned to 32 bytes for AVX-256)
        }

        case MirTypeKind::Pointer:
            return 8; // x64 pointers align to 8-byte boundaries

        case MirTypeKind::Array:
            // Array alignment is the alignment of its underlying element type
            if (!type->getSubTypes().empty())
            {
                return getTypeAlignmentInBytes(type->getSubTypes().front());
            }
            return 1;

        default:
            return 1;
    }
}

size_t EzMirTestSuiteTypeLayout::getTypeSizeInBytes(const MirType *type) const
{
    if (!type)
        return 0;

    switch (type->getKind())
    {
        case MirTypeKind::Void:
            return 0; // Void has no size

        case MirTypeKind::Integer:
        case MirTypeKind::FloatingPoint:
        {
            // Return size in bytes rounded up to the nearest byte boundary
            return (type->getTotalSizeInBits() + 7) / 8;
        }

        case MirTypeKind::Pointer:
            return 8;

        case MirTypeKind::Array:
        {
            // Array size is the padded size of its element * element count
            if (!type->getSubTypes().empty())
            {
                const MirType *elementType = type->getSubTypes().front();
                size_t elementSize = getTypeSizeInBytes(elementType);

                // Retrieve element count by dividing the array's overall bit-size representation
                // by the single element's size in bits
                size_t elementSizeInBits = elementType->getTotalSizeInBits();
                if (elementSizeInBits > 0)
                {
                    size_t elementCount = type->getTotalSizeInBits() / elementSizeInBits;
                    return elementSize * elementCount;
                }
            }
            return 0;
        }

        default:
            return 0;
    }
}