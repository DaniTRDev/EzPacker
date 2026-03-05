/**
 * @file MirType.h
 * @brief Lightweight, self-contained type representation for the MIR layer.
 *
 * MirType is intentionally decoupled from the semantic-layer Type class.
 * It describes primitive kinds (Integer, FloatingPoint, Pointer, Void) and
 * compound kinds (Array) with an optional list of sub-types.  Every MirType
 * has a unique ID assigned by MirEmitterContext and a human-readable name
 * for debugging.
 *
 * This class is kept trivially destructible so it can live inside a
 * TypedPool arena alongside other MIR objects.
 */
#ifndef EZPACKER_MIRTYPE_H
#define EZPACKER_MIRTYPE_H

#include "EzMirCommon.h"

enum class MirTypeKind
{
    Invalid = 0,
    Integer,
    FloatingPoint,
    Pointer,
    Array, // An array of other types.
    Void
};

/**
 * Represents a type in the MIR (Mid-level Intermediate Representation). This class is completely decoupled from
 * EzSemantics/Scope/Type and is designed to be a simple, self-contained representation of types for the MIR. It can
 * represent basic types (like int, float), compound types (like structs, arrays), and function types. It should be
 * designed to be easily serializable and should not contain any complex logic or dependencies on other parts of the
 * system.
 *
 * This class MUST BE KEPT TRIVIALLY DESTRUCTIBLE to allow for efficient storage in containers and easy serialization.
 */
class MirType
{
  public:
    /***
     * Constructs a MirType with the given kind, id, subTypes and name.
     * @param kind
     * @param id
     * @param subTypes For compound types (like structs, arrays), this can hold the subtypes. For basic types, this can
     * be null.
     * @param name
     */
    MirType(MirTypeKind kind, size_t id, TypedPoolSlice<MirType> *subTypes, const std::string_view &name);

    /**
     * Returns the kind of the type (e.g., Integer, Float, Pointer, Array, Void).
     * @return MirTypeKind
     */
    MirTypeKind getKind() const;

    /**
     * Returns the unique identifier for the type. This can be used to distinguish between different types, especially
     * when
     * @return size_t
     */
    size_t getId() const;

    /**
     * Returns the subtypes of this type if it is a compound type (like struct or array). For basic types, this will be
     * @return TypedPoolSlice<MirType> *
     */
    TypedPoolSlice<MirType> *getSubTypes() const;

    /**
     * Returns the name of the type (e.g., "int", "float", "MyStruct"). This is primarily for debugging and
     * serialization purposes.
     * @return const std::string_view &
     */
    const std::string_view &getName() const;

  private:
    MirTypeKind m_kind;                  // The kind of the type.
    size_t m_id;                         // Unique identifier for the type
    TypedPoolSlice<MirType> *m_subTypes; // For compound types (like structs, arrays), this can hold the subtypes. For
                                         // basic types, this can be null.
    std::string_view m_name;             // Name of the type (e.g., "int", "float", "MyStruct")
};

#endif // EZPACKER_MIRTYPE_H
