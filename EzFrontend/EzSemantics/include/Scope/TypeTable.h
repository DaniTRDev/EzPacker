#ifndef EZPACKER_TYPETABLE_H
#define EZPACKER_TYPETABLE_H

#include "EzSemanticsCommon.h"

enum class UnderlyingType : uint8_t
{
    Invalid = 0,
    FloatingPoint,
    Integer,
    Pointer,
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
    Variable = 0xFFFF // Strings, they don't have a pre-fixed known size or ptrs.
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
    Type(UnderlyingType underlyingType, UnderlyingTypeSize underlyingTypeSize, const std::string &typeName);

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
     * @return const std::string &
     */
    const std::string &getTypeName() const;

  private:
    UnderlyingType m_underlyingType;
    UnderlyingTypeSize m_underlyingTypeSize;
    std::string m_typeName;
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
    static bool doesTypeExists(const std::string &typeName);

    /**
     * Returns a type, if exists, of the given typeName. Returns true if type does not exist.
     * @param typeName
     * @return bool
     */
    static std::shared_ptr<Type> getType(const std::string &typeName);

  private:
    static std::map<std::string, std::shared_ptr<Type>> m_types;
};

#endif // EZPACKER_TYPETABLE_H
