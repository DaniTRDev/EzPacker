#include "Type/EzTestTripleTypeLayout.h"

size_t EzTestTripleTypeLayout::getPointerSizeInBytes() const { return 8; }

size_t EzTestTripleTypeLayout::getTypeAlignmentInBytes(const MirType *type) const
{
    if (!type)
        return 1;
    switch (type->getKind())
    {
        case MirTypeKind::Integer:
        case MirTypeKind::FloatingPoint:
        {
            size_t bits = type->getTotalSizeInBits();
            if (bits <= 8)
                return 1;
            if (bits <= 16)
                return 2;
            if (bits <= 32)
                return 4;
            if (bits <= 64)
                return 8;
            return 16; // 128-bit types aligned to 16 bytes
        }
        case MirTypeKind::Pointer:
            return 8;
        case MirTypeKind::Array:
            return getTypeAlignmentInBytes(type->getArrayElementType());
        case MirTypeKind::Class:
        {
            // For classes, the total size of the type is calculated with target's alignmemt uppon class creation.
            return type->getMaxAlignmentInBytes();
        }
        default:
            return 1;
    }
}

size_t EzTestTripleTypeLayout::getTypeSizeInBytes(const MirType *type) const
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

        case MirTypeKind::Class:
        {
            // If it's a pre-constructed struct, return its already aligned size.
            // Our MirTypeTable::getClass handles this padding logic upon creation.
            return (type->getTotalSizeInBits() + 7) / 8;
        }

        default:
            return 0;
    }
}