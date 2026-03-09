/**
 * @file TypeTable.h
 * @brief Built-in semantic type system used by EzSemantics.
 *
 * `Type` models one semantic type known to the front-end. `TypeTable` is the
 * shared registry used by semantic passes to resolve source-level type names
 * into canonical `Type` objects.
 *
 * According to the current implementation, the built-in table contains:
 *   - floating-point types: `float`, `double`
 *   - integer types: `i8`, `i16`, `i32`, `i64`, `i128`, `i256`, `i512`
 *   - `string`
 *   - `void`
 *
 * The default semantic type returned by `getDefaultType()` is `i64`.
 */
#ifndef EZPACKER_TYPETABLE_H
#define EZPACKER_TYPETABLE_H

#include "EzSemanticsCommon.h"

/**
 * Broad runtime category of a semantic type.
 */
enum class UnderlyingType : uint8_t
{
    Invalid = 0,
    FloatingPoint,
    Integer,
    String,
    Void
};

/**
 * Storage width used by a semantic type.
 *
 * `Variable` is used for types such as `string`, whose size is not represented
 * as a fixed integer bit-width in this layer.
 */
enum class UnderlyingTypeSize : uint16_t
{
    Invalid = 0,
    _8bits = 8,
    _16bits = 16,
    _32bits = 32,
    _64bits = 64,
    _128bits = 128,
    _256bits = 256,
    _512bits = 512,
    Variable = 0xFFFF // Strings, they don't have a pre-fixed known size.
};

class Type
{
  public:
    /**
     * Creates one semantic type descriptor.
     */
    Type(UnderlyingType underlyingType, UnderlyingTypeSize underlyingTypeSize, const std::string_view &typeName);

    /**
     * Returns the broad runtime category of this type.
     */
    UnderlyingType getUnderlyingType() const;

    /**
     * Returns the storage width associated with this type.
     */
    UnderlyingTypeSize getUnderlyingTypeSize() const;

    /**
     * Returns the canonical source-level name of this type.
     */
    const std::string_view &getTypeName() const;

  private:
    UnderlyingType m_underlyingType;
    UnderlyingTypeSize m_underlyingTypeSize;
    std::string_view m_typeName;
};

/**
 * Static registry of all semantic types recognised by the front-end.
 */
class TypeTable
{
  public:
    /**
     * Returns whether a semantic type with the given canonical name exists.
     */
    static bool doesTypeExists(const std::string_view &typeName);

    /**
     * Returns the canonical `Type` object for the given name, or `nullptr` if
     * the type is unknown.
     */
    static std::shared_ptr<Type> getType(const std::string_view &typeName);

    /**
     * Returns the language default semantic type, currently `i64`.
     */
    static std::shared_ptr<Type> getDefaultType();

  private:
    static std::map<std::string_view, std::shared_ptr<Type>> m_types;
};

#endif // EZPACKER_TYPETABLE_H
