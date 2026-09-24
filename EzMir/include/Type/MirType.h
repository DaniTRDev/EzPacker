#ifndef EZMIR_MIR_TYPE_H
#define EZMIR_MIR_TYPE_H

#include "EzMirCommon.h"

/**
 * High-level categorization of MIR types.
 */
enum class MirTypeKind : uint8_t
{
    Invalid = 0,   // Uninitialized or invalid type sentinel
    Integer,       // Fixed-width integer types (e.g., i8, i16, i32, i64, i128)
    FloatingPoint, // IEEE floating-point types (e.g., f32, f64, f128)
    Function,      // Function signature type encapsulating return and parameter types
    Pointer,       // Pointer type referencing a pointee MirType
    Array,         // Homogeneous sequential collection of elements of a base type (e.g., i32[3])
    Void,          // Unit or empty type representing lack of value
    BindingToken,  // Token type used for control-flow or lowering bindings (e.g., PUSH_ARGS to CALL, PUSH_RET to RET)
    Vector         // Vector SIMD type (e.g. v4f32, v2f64, v4i32, 128/256/512 bits)
};

/**
 * Represents one MIR type record stored in the emitter context's type pool.
 *
 * The object is a plain descriptor. It performs no semantic validation on its
 * own and does not resolve names; callers are expected to build valid type
 * graphs before referencing them from functions or instructions.
 */
class MirType
{
  public:
    /**
     * Constructs a MIR type descriptor.
     */
    MirType(MirTypeKind kind,
            class MirTypeTable *owner,
            size_t id,
            size_t maxAlignmentInBits,
            size_t totalSizeInBits,
            std::pmr::string name,
            std::pmr::vector<MirType *> subTypes,
            uint8_t compactId);

    /**
     * Returns true if the type has zero required byte alignment.
     */
    bool isUnAligned() const;

    /**
     * Returns the element type if this type is an array kind, or nullptr otherwise.
     */
    MirType *getArrayElementType() const;

    /**
     * Returns the pointee type if this type is a pointer kind, or nullptr otherwise.
     */
    MirType *getPointedType() const;

    /**
     * Returns the high-level category of this type.
     */
    MirTypeKind getKind() const;

    /**
     * Returns the parent type table owning this type descriptor.
     */
    class MirTypeTable *getOwner() const;

    /**
     * Returns the unique MIR identifier of this type descriptor.
     */
    size_t getId() const;

    /**
     * Returns the maximum alignment requirement in bits.
     */
    size_t getMaxAlignmentInBits() const;

    /**
     * Returns the total bit width of this type.
     */
    size_t getTotalSizeInBits() const;

    /**
     * Returns the total byte width of this type (bits divided by 8).
     */
    size_t getTotalSizeInBytes() const;

    /**
     * Return the compact ID of this type.
     */
    uint8_t getCompactId() const;

    /**
     * Returns the human-readable diagnostic name associated with this type.
     * Identity is determined by getId() rather than name.
     */
    const std::pmr::string &getName() const;

    /**
     * Returns the collection of subtype descriptors for compound kinds (e.g. array element or pointee type).
     */
    const std::pmr::vector<MirType *> &getSubTypes() const;

  private:
    /**
     * High-level classification of the type.
     */
    MirTypeKind m_kind;

    /**
     * Type table managing the lifetime and lookup of this type.
     */
    class MirTypeTable *m_owner;

    /**
     * Unique MIR identifier for this type record.
     */
    size_t m_id;

    /**
     * Byte alignment requirement for memory storage of this type.
     */
    size_t m_maxAlignmentInBits;

    /**
     * Total bit width of this type.
     */
    size_t m_totalSizeInBits;

    /**
     * Returns the compact type id of this type for fast index lookups.
     */
    uint8_t m_compactId;

    /**
     * Informational/diagnostic type name string.
     */
    std::pmr::string m_name;

    /**
     * Subordinate types for compound constructs (pointees, element types, etc.).
     */
    std::pmr::vector<MirType *> m_subTypes;
};

#endif // EZPACKER_MIRTYPE_H
