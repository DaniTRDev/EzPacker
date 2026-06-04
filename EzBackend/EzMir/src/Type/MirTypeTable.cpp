#include "Type/MirTypeTable.h"

MirTypeTable::MirTypeTable(std::pmr::memory_resource *globalArena) :
    m_arena(globalArena), m_typeNames(globalArena), m_idToType(globalArena), m_pointerCache(globalArena)
{
}

MirType *MirTypeTable::create(MirTypeKind kind,
                              size_t totalSizeInBytes,
                              std::pmr::vector<MirType *> subTypes,
                              const std::string_view &name)
{
    if (name.empty())
        throw std::runtime_error("Compiler Failure: Type initialization requires a valid diagnostic name.");

    std::pmr::string lookupName(name, m_arena);
    auto it = m_typeNames.find(lookupName);
    if (it != m_typeNames.end())
    {
        return it->second; // Type safety: return existing canonical type match
    }

    // Allocate our node container explicitly out of the stable global metadata resource arena
    std::pmr::polymorphic_allocator<MirType> alloc(m_arena);
    MirType *uniqueType = alloc.allocate(1);

    size_t assignedId = ++m_currentId;
    alloc.construct(uniqueType, kind, assignedId, totalSizeInBytes, lookupName, std::move(subTypes));

    // Store in our fast global indexing maps
    m_typeNames[lookupName] = uniqueType;
    m_idToType[assignedId] = uniqueType;

    return uniqueType;
}

MirType *MirTypeTable::getPtr(MirType *srcType)
{
    if (!srcType)
        return nullptr;

    // Check type cache map first (Interning check)
    auto it = m_pointerCache.find(srcType);
    if (it != m_pointerCache.end())
    {
        return it->second;
    }

    // Assuming a 64-bit compilation target backend environment layout
    constexpr size_t pointerSizeInBytes = 8;

    std::pmr::vector<MirType *> childTarget({ srcType }, m_arena);
    std::string formattedName = std::format("{}*", srcType->getName());

    MirType *newPointerType = create(MirTypeKind::Pointer, pointerSizeInBytes, std::move(childTarget), formattedName);

    // Save inside cache table for future deduplication lookups
    m_pointerCache[srcType] = newPointerType;
    return newPointerType;
}

MirType *MirTypeTable::getArray(MirType *elementType, size_t elementCount)
{
    if (!elementType)
        return nullptr;

    // Generate string signature for this array shape: "i32[10]" or "i8*[4]".
    std::string arraySignature = std::format("{}[{}]", elementType->getName(), elementCount);
    std::pmr::string lookupName(arraySignature, m_arena);

    // Check type names cache to see if this exact array shape already exists (Interning check).
    auto it = m_typeNames.find(lookupName);
    if (it != m_typeNames.end())
    {
        return it->second; // Return the existing type.
    }

    size_t totalSizeInBytes = elementType->getTotalSizeInBytes() * elementCount;
    std::pmr::vector<MirType *> childType({ elementType }, m_arena);

    // Instantiate and construct the unique Array type record on the global arena
    std::pmr::polymorphic_allocator<MirType> alloc(m_arena);
    MirType *newArrayType = alloc.allocate(1);

    size_t assignedId = ++m_currentId;
    alloc.construct(newArrayType, MirTypeKind::Array, assignedId, totalSizeInBytes, lookupName, std::move(childType));

    m_typeNames[lookupName] = newArrayType;
    m_idToType[assignedId] = newArrayType;

    return newArrayType;
}

MirType *MirTypeTable::getMirTypeById(size_t id) const
{
    auto it = m_idToType.find(id);
    return (it != m_idToType.end()) ? it->second : nullptr;
}

MirType *MirTypeTable::getStruct(std::pmr::vector<MirType *> fieldTypes, const std::string_view &structName)
{
    // Calculate total size based on type alignment rules
    size_t totalSize = 0;
    for (auto *field : fieldTypes)
    {
        // Simple alignment rule: align to field size
        totalSize = (totalSize + (field->getTotalSizeInBytes() - 1)) & ~(field->getTotalSizeInBytes() - 1);
        totalSize += field->getTotalSizeInBytes();
    }

    return create(MirTypeKind::Struct, totalSize, std::move(fieldTypes), structName);
}

MirType *MirTypeTable::getVoidType() const { return m_voidType; }
MirType *MirTypeTable::getInt1Type() const { return m_int1Type; }
MirType *MirTypeTable::getInt8Type() const { return m_int8Type; }
MirType *MirTypeTable::getInt16Type() const { return m_int16Type; }
MirType *MirTypeTable::getInt32Type() const { return m_int32Type; }
MirType *MirTypeTable::getInt64Type() const { return m_int64Type; }

MirType *MirTypeTable::getFloat32Type() const { return m_float32Type; }

MirType *MirTypeTable::getFloat64Type() const { return m_float64Type; }

void MirTypeTable::initialize()
{
    m_voidType = create(MirTypeKind::Void, 0, {}, "void");
    m_int1Type = create(MirTypeKind::Integer, 1, {}, "i1");
    m_int8Type = create(MirTypeKind::Integer, 1, {}, "i8");
    m_int16Type = create(MirTypeKind::Integer, 2, {}, "i16");
    m_int32Type = create(MirTypeKind::Integer, 4, {}, "i32");
    m_int64Type = create(MirTypeKind::Integer, 8, {}, "i64");
    m_float32Type = create(MirTypeKind::FloatingPoint, 4, {}, "f32");
    m_float64Type = create(MirTypeKind::FloatingPoint, 8, {}, "f64");
}
