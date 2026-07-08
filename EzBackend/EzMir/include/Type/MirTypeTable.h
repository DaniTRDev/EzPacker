#ifndef EZPACKER_MIRTYPETABLE_H
#define EZPACKER_MIRTYPETABLE_H

#include "EzCoreCommon.h"
#include "MirType.h"

/**
 * Class used to store the least minimum required types so everything else works.
 */
class MirTypeTable
{
  public:
    /**
     * Creates the type table with the given arena allocator.
     * @param globalArena
     */
    MirTypeTable(std::pmr::memory_resource *globalArena);

    // Disable copies to safeguard our arena resource mappings
    MirTypeTable(const MirTypeTable &) = delete;
    MirTypeTable &operator=(const MirTypeTable &) = delete;

    /**
     * Creates a unique base or compound type.
     * @param kind
     * @param totalSizeInBits
     * @param subTypes
     * @param name
     * @return
     */
    MirType *create(MirTypeKind kind,
                    size_t totalSizeInBits,
                    std::pmr::vector<MirType *> subTypes,
                    const std::string_view &name);

    /**
     * Interns pointer types. Guarantees that getPtr(T) always returns the exact same type instance pointer.
     * @param srcType
     * @return
     */
    MirType *getPtr(MirType *srcType);

    /**
     * Interns array types structurally based on size and base element composition.
     * @param elementType
     * @param elementCount
     * @return
     */
    MirType *getArray(MirType *elementType, size_t elementCount);

    /**
     * Searches the table for the given ID and returns its type, if any. Returns nullptr if type was not created.
     * @param id
     * @return
     */
    MirType *getMirTypeById(size_t id) const;

    /**
     * Creates an struct type (if it does not exist). Returns the existing type if it was already created.
     * @param fieldTypes
     * @param structName
     * @return
     */
    MirType *getStruct(std::pmr::vector<MirType *> fieldTypes, const std::string_view &structName);

    MirType *getVoidType() const;
    MirType *i1() const;
    MirType *i8() const;
    MirType *i16() const;
    MirType *i32() const;
    MirType *i64() const;
    MirType *f32() const;
    MirType *f64() const;

    /**
     * Initializes the type table with the basic primitive types needed: iXX, void and fXX.
     */
    void initialize();

  private:
    std::pmr::memory_resource *m_arena;
    size_t m_currentId{ 0 };

    // Built-in basic primitives
    MirType *m_voidType{ nullptr };
    MirType *m_int1Type{ nullptr };
    MirType *m_int8Type{ nullptr };
    MirType *m_int16Type{ nullptr };
    MirType *m_int32Type{ nullptr };
    MirType *m_int64Type{ nullptr };
    MirType *m_float32Type{ nullptr };
    MirType *m_float64Type{ nullptr };

    // High performance tracking hashes using PMR mapping blocks
    std::pmr::unordered_map<std::pmr::string, MirType *> m_typeNames;
    std::pmr::unordered_map<size_t, MirType *> m_idToType;

    // Interning caches: maps base type to its unique pointer type representation
    std::pmr::unordered_map<MirType *, MirType *> m_pointerCache;
};

#endif // EZPACKER_MIRTYPETABLE_H
