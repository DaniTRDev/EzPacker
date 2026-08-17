#ifndef EZMIR_MIR_TYPE_TABLE_H
#define EZMIR_MIR_TYPE_TABLE_H

#include "EzCoreCommon.h"
#include "Type/MirType.h"

/**
 * Class used to store the least minimum required types so everything else works.
 *
 * IMPORTANT: When created, types are aligned using the given target type layout (primitives).
 */
class MirTypeTable
{
  public:
    /**
     * Creates the type table with the given target type layout and arena allocator.
     */
    MirTypeTable(std::pmr::memory_resource *globalArena);

    // Disable copies to safeguard our arena resource mappings
    MirTypeTable(const MirTypeTable &) = delete;
    MirTypeTable &operator=(const MirTypeTable &) = delete;

    /**
     * Returns the target type layout used by this type table.
     */
    class IMirTargetTypeLayout *getTargetTypeLayout() const;

    /**
     * Creates a unique base or compound type. If there was an error while creating the type nullptr is returned.
     */
    MirType *create(MirTypeKind kind,
                    size_t totalSizeInBits,
                    std::pmr::vector<MirType *> subTypes,
                    const std::string_view &name);

    /**
     * Returns a type that is only used to bind things, see more at MirTypeKind.
     */
    MirType *getBindingToken() const;

    /**
     * Creates a class type (if it does not exist). Returns the existing type if it was already created or nullptr if
     * there was any error.
     */
    MirType *getClass(const std::pmr::vector<MirType *> &fieldTypes, const std::string_view &name);

    /**
     * Creates a function type with the given parameters. This function will CREATE only if it wasn't added before, if
     * it was the existing type is returned.
     */
    MirType *getFuncType(MirType *returnType,
                         const std::pmr::list<class MirRegister *> &parameters,
                         const std::string_view &funcName);

    /**
     * Interns pointer types. Guarantees that getPtr(T) always returns the exact same type instance pointer.
     */
    MirType *getPtr(MirType *srcType);

    /**
     * Interns array types structurally based on size and base element composition.
     */
    MirType *getArray(MirType *elementType, size_t elementCount);

    /**
     * Returns the first floating-point type that can hold the given bit size.
     */
    MirType *getFloatingTypeBySize(size_t sizeInBits) const;

    /**
     * Returns the first integer type that can hold the given bit size.
     */
    MirType *getIntegerTypeBySize(size_t sizeInBits) const;

    /**
     * Searches the table for the given ID and returns its type, if any. Returns nullptr if type was not created.
     */
    MirType *getMirTypeById(size_t id) const;

    MirType *getVoidType();
    MirType *i1();
    MirType *i8();
    MirType *i16();
    MirType *i32();
    MirType *i64();
    MirType *i128();
    MirType *i256();

    MirType *f32();
    MirType *f64();
    MirType *f128();

    /**
     * Initializes the type table with the basic primitive types needed as well as the type layout class.
     */
    void initialize(class IMirTargetTypeLayout *typeLayout);

  private:
    class IMirTargetTypeLayout *m_typeLayout{ nullptr };
    size_t m_currentId{ 0 };

    MirType *m_bindingToken{ nullptr };

    // Built-in basic primitives
    MirType *m_voidType{ nullptr };
    MirType *m_int1Type{ nullptr };
    MirType *m_int8Type{ nullptr };
    MirType *m_int16Type{ nullptr };
    MirType *m_int32Type{ nullptr };
    MirType *m_int64Type{ nullptr };
    MirType *m_int128Type{ nullptr };
    MirType *m_int256Type{ nullptr };

    MirType *m_float32Type{ nullptr };
    MirType *m_float64Type{ nullptr };
    MirType *m_float128Type{ nullptr };

    std::pmr::memory_resource *m_arena;

    // High performance tracking hashes using PMR mapping blocks
    std::pmr::unordered_map<std::pmr::string, MirType *> m_typeNames;
    std::pmr::unordered_map<size_t, MirType *> m_idToType;

    // Interning caches: maps base type to its unique pointer type representation
    std::pmr::unordered_map<MirType *, MirType *> m_pointerCache;
};

#endif // EZMIR_MIR_TYPE_TABLE_H
