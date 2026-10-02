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
 * Runtime Type Information (RTTI) descriptor capturing type identities, hierarchy, and declaration coordinates.
 */
struct RttiTypeDescriptor
{
    uint64_t typeId{ 0 };                              ///< 64-bit unique type hash (e.g. FNV-1a).
    std::string_view typeName;                         ///< Demangled or qualified type name.
    uint32_t numBases{ 0 };                            ///< Number of direct base types.
    const RttiTypeDescriptor *const *bases{ nullptr }; ///< Base type array for hierarchical subtyping checks.
    SourceRefData declarationSite;                     ///< Location where type was defined.

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

