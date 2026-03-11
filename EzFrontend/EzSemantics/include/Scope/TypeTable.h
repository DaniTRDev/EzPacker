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
    Void,
    Module // Used to identify this type derived from a module.
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
    Variable = 0xFFFF // Strings or Modules, they don't have a pre-fixed known size.
};

class Type
{
  public:
    /**
     * Creates one semantic type descriptor.
     */
    Type(UnderlyingType underlyingType, UnderlyingTypeSize underlyingTypeSize, const std::string_view &typeName);

    /**
     * Returns the source reference (if exists) that defined this type. Caller should check m_valid to ensure it
     * contains valid data.
     */
    const SourceReference &getSourceRef() const;

    /**
     * Returns the broad runtime category of this type.
     */
    UnderlyingType getUnderlyingType() const;

    /**
     * Returns the storage width associated with this type.
     */
    UnderlyingTypeSize getUnderlyingTypeSize() const;

    /**
     * Sets the source reference of where this type has been defined.
     * @param ref
     */
    void setSourceRef(const SourceReference &ref);

    /**
     * Sets the subtypes replacing the previous ones, if any.
     */
    void setSubTypes(const std::vector<Type *> &types);

    /**
     * Returns the canonical source-level name of this type.
     */
    const std::string_view &getTypeName() const;

    /**
     * Returns the subtypes of this type. This field will only be != empty when the underlying type is a Module.
     */
    const std::vector<Type *> &getSubTypes() const;

  private:
    SourceReference m_sourceRef;
    UnderlyingType m_underlyingType;
    UnderlyingTypeSize m_underlyingTypeSize;
    std::string_view m_typeName;
    std::vector<Type *> m_subTypes;
};

/**
 * Static registry of all semantic types recognised by the front-end.
 */
class TypeTable
{
  public:
    /**
     * Creates the default type table with privitive types defined.
     */
    TypeTable();

    /**
     * Returns whether a semantic type with the given canonical name exists.
     */
    bool doesTypeExists(const std::string_view &typeName);

    /**
     * Returns the canonical `Type` object for the given name, or `nullptr` if
     * the type is unknown.
     */
    std::shared_ptr<Type> getType(const std::string_view &typeName);

    /**
     * Adds a type into the type table.
     *
     * A valid `std::shared_ptr<Type>` instance if type did not exist and could be added. Returns `nullptr` if
     * the type already exists.
     */
    std::shared_ptr<Type>
    addType(UnderlyingType underlyingType, UnderlyingTypeSize underlyingTypeSize, const std::string_view &typeName);

    /**
     * Creates a module type and sets its subtypes.
     *
     * A valid `std::shared_ptr<Type>` instance if type did not exist and could be added. Returns `nullptr` if
     * the type already exists.
     */
    std::shared_ptr<Type> addModuleType(const std::string_view &moduleName, const std::vector<Type *> &subTypes);

    /**
     * Returns the language default semantic type, currently `i64`.
     */
    std::shared_ptr<Type> getDefaultType();

    /**
     * Returns the internal type list used by this class.
     */
    const std::map<std::string_view, std::shared_ptr<Type>> &getTypeMap() const;

  private:
    std::map<std::string_view, std::shared_ptr<Type>> m_types;
};

#endif // EZPACKER_TYPETABLE_H
