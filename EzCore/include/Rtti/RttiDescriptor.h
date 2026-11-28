#ifndef EZCORE_RTTI_DESCRIPTOR_H
#define EZCORE_RTTI_DESCRIPTOR_H

#include <string_view>
#include <cstdint>

namespace EzCore
{

/**
 * Encapsulates source reference coordinates (file path, line, column, function, message snippet)
 * captured at exception throw sites or type definition sites.
 */
struct SourceRefData
{
    std::string_view filePath;     ///< Path to origin source file.
    std::string_view functionName; ///< Enclosing function name.
    uint32_t line{ 0 };            ///< 1-based source line.
    uint32_t column{ 0 };          ///< 1-based source column.
    std::string_view snippet;      ///< Contextual excerpt or diagnostic text.
};

/**
 * High-level structural kind of a type described by RTTI or debug information.
 */
enum class RttiTypeKind : uint8_t
{
    Unknown = 0,
    Void,
    Boolean,
    SignedInt,
    UnsignedInt,
    Float,
    Pointer,
    Array,
    Struct,
    Union,
    Enum,
    Function,
    Vector,
    Typedef
};

/**
 * Describes a single field/member within a composite RTTI type (struct or union).
 */
struct RttiMemberDescriptor
{
    std::string_view name;     ///< Field identifier.
    uint64_t typeId{ 0 };      ///< 64-bit type hash of the member's type.
    uint32_t byteOffset{ 0 };  ///< Byte offset from the start of the enclosing aggregate.
    uint16_t bitOffset{ 0 };   ///< Bit offset within the storage unit for bitfields.
    uint16_t bitWidth{ 0 };    ///< Width in bits (0 for non-bitfield members).
};

/**
 * Runtime Type Information (RTTI) descriptor capturing type identities, hierarchy, and declaration coordinates.
 */
struct RttiTypeDescriptor
{
    uint64_t typeId{ 0 };                              ///< 64-bit unique type hash (e.g. FNV-1a).
    std::string_view typeName;                         ///< Demangled or qualified type name.
    uint32_t numBases{ 0 };                            ///< Number of direct base types.
    const RttiTypeDescriptor *const *bases{ nullptr }; ///< Base type array for hierarchical subtyping checks.
    SourceRefData declarationSite;                     ///< Location where type was defined.
    uint64_t sizeInBytes{ 0 };                         ///< Size of the type in bytes (0 if unsized/unknown).
    uint32_t alignment{ 0 };                           ///< Alignment requirement in bytes (0 if unknown).
    RttiTypeKind kind{ RttiTypeKind::Unknown };        ///< Structural kind of the type.
    uint32_t numMembers{ 0 };                          ///< Number of member descriptors (for struct/union).
    const RttiMemberDescriptor *members{ nullptr };    ///< Member descriptor array (nullptr if none).

    /**
     * Checks if this type is identical to or derives from the target base type.
     */
    bool isA(const RttiTypeDescriptor *target) const
    {
        if (!target)
        {
            return false;
        }
        if (this == target || typeId == target->typeId)
        {
            return true;
        }
        for (uint32_t i = 0; i < numBases; ++i)
        {
            if (bases && bases[i] && bases[i]->isA(target))
            {
                return true;
            }
        }
        return false;
    }
};

/**
 * Exception payload carrying the value/pointer, type descriptor (optional in --no-rtti),
 * and throw-site source coordinates.
 */
struct RichExceptionPayload
{
    void *payload{ nullptr };
    const RttiTypeDescriptor *rtti{ nullptr };
    SourceRefData throwSite;
};

/**
 * FNV-1a 64-bit hash computation for type names.
 */
inline constexpr uint64_t computeTypeId(std::string_view name)
{
    uint64_t hash = 14695981039346656037ULL;
    for (char c : name)
    {
        hash ^= static_cast<uint8_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

inline constexpr std::string_view kDefaultExceptionTypeName = "EzDefaultException";
inline constexpr uint64_t kDefaultExceptionTypeId = computeTypeId(kDefaultExceptionTypeName);

} // namespace EzCore

#endif // EZCORE_RTTI_DESCRIPTOR_H

