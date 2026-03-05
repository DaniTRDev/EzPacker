/**
 * @file TypeTable.h
 * @brief The language's built-in type registry: `i8`, `i16`, `i32`, `i64`, `f32`, `f64`, `void`, etc.
 *
 * Type describes a single primitive type (underlying kind + bit-width +
 * human-readable name).  TypeTable is a static lookup that maps type-name
 * strings to their Type objects and provides a default type (`i64`) used
 * when no explicit type annotation is given.
 */
#ifndef EZPACKER_TYPETABLE_H
#define EZPACKER_TYPETABLE_H

#include "EzSemanticsCommon.h"

enum class UnderlyingType : uint8_t
{
    Invalid = 0,
    FloatingPoint,
    Integer,
    String,
    Void
};

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
     * Creates a new type object with the given parameters.
     * @param underlyingType
     * @param underlyingTypeSize
     * @param typeName
     */
    Type(UnderlyingType underlyingType, UnderlyingTypeSize underlyingTypeSize, const std::string_view &typeName);

    /**
     * Returns the underlying type of this type object.
     * @return UnderlyingType
     */
    UnderlyingType getUnderlyingType() const;

    /**
     * Returns the underlying type size of this type object.
     * @return UnderlyingType
     */
    UnderlyingTypeSize getUnderlyingTypeSize() const;

    /**
     * Returns the name of the type.
     * @return const std::string_view &
     */
    const std::string_view &getTypeName() const;

  private:
    UnderlyingType m_underlyingType;
    UnderlyingTypeSize m_underlyingTypeSize;
    std::string_view m_typeName;
};

/**
 * This class contains a table of allowed types in the language. Made this structure static for convenient access.
 */
class TypeTable
{
  public:
    /**
     * Returns true if the given type exists.
     * @param typeName
     * @return bool
     */
    static bool doesTypeExists(const std::string_view &typeName);

    /**
     * Returns a type, if exists, of the given typeName. Returns true if type does not exist.
     * @param typeName
     * @return bool
     */
    static std::shared_ptr<Type> getType(const std::string_view &typeName);

    /**
     * Returns the default type for the compiler.
     * @return std::shared_ptr<Type>
     */
    static std::shared_ptr<Type> getDefaultType();

  private:
    static std::map<std::string_view, std::shared_ptr<Type>> m_types;
};

#endif // EZPACKER_TYPETABLE_H
