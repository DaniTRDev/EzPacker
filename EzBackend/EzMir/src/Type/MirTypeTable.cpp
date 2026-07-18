#include "Type/MirTypeTable.h"

MirTypeTable::MirTypeTable(IMirTargetTypeLayout *typeLayout, std::pmr::memory_resource *globalArena) :
    m_typeLayout(typeLayout), m_arena(globalArena), m_typeNames(globalArena), m_idToType(globalArena),
    m_pointerCache(globalArena)
{
}

IMirTargetTypeLayout *MirTypeTable::getTargetTypeLayout() const { return m_typeLayout; }

MirType *MirTypeTable::create(MirTypeKind kind,
                              size_t totalSizeInBits,
                              std::pmr::vector<MirType *> subTypes,
                              const std::string_view &name)
{
    if (name.empty())
    {
        return nullptr;
    }

    std::pmr::string lookupName(name, m_arena);
    auto it = m_typeNames.find(lookupName);
    if (it != m_typeNames.end())
    {
        return it->second;
    }

    size_t assignedId = ++m_currentId;

    // Allocate a temporary dummy node record to fetch target-specific alignment layouts
    // Primitive fields don't have sub-types populated yet, so we query based on kind + size
    size_t alignmentInBytes = 1;
    if (kind == MirTypeKind::Integer || kind == MirTypeKind::FloatingPoint || kind == MirTypeKind::Void)
    {
        // Allocate space mapping primitive target metrics
        MirType tempPrimitive(kind, assignedId, 1, totalSizeInBits, lookupName, {});
        alignmentInBytes = m_typeLayout->getTypeAlignmentInBytes(&tempPrimitive);
    }
    else if (kind == MirTypeKind::Pointer || kind == MirTypeKind::Function)
    {
        alignmentInBytes = m_typeLayout->getPointerSizeInBytes();
    }

    // Allocate our node container explicitly out of the stable global metadata resource arena
    std::pmr::polymorphic_allocator<MirType> alloc(m_arena);
    MirType *uniqueType = alloc.new_object<MirType>(kind,
                                                    assignedId,
                                                    alignmentInBytes,
                                                    totalSizeInBits,
                                                    lookupName,
                                                    std::move(subTypes));

    m_typeNames[lookupName] = uniqueType;
    m_idToType[assignedId] = uniqueType;

    return uniqueType;
}

MirType *MirTypeTable::getBindingToken() const
{
    return m_bindingToken;
}

MirType *MirTypeTable::getClass(const std::pmr::vector<MirType *> &fieldTypes, const std::string_view &structName)
{
    if (structName.empty())
    {
        return nullptr;
    }

    std::pmr::string lookupName(structName, m_arena);
    auto it = m_typeNames.find(lookupName);
    if (it != m_typeNames.end())
    {
        return it->second;
    }

    size_t currentOffsetInBytes = 0;
    size_t maxAlignmentInBytes = 1;

    for (const auto *fieldType : fieldTypes)
    {
        if (!fieldType)
            continue;

        size_t fieldAlignment = m_typeLayout->getTypeAlignmentInBytes(fieldType);
        size_t fieldSize = m_typeLayout->getTypeSizeInBytes(fieldType);

        if (fieldAlignment > maxAlignmentInBytes)
        {
            maxAlignmentInBytes = fieldAlignment;
        }

        if (currentOffsetInBytes % fieldAlignment != 0)
        {
            currentOffsetInBytes += (fieldAlignment - (currentOffsetInBytes % fieldAlignment));
        }

        currentOffsetInBytes += fieldSize;
    }

    if (currentOffsetInBytes % maxAlignmentInBytes != 0)
    {
        currentOffsetInBytes += (maxAlignmentInBytes - (currentOffsetInBytes % maxAlignmentInBytes));
    }

    size_t totalSizeInBits = currentOffsetInBytes * 8;

    // Custom allocation route bypasses 'create' to pass the explicitly calculated struct layout properties directly
    size_t assignedId = ++m_currentId;
    std::pmr::polymorphic_allocator<MirType> alloc(m_arena);
    MirType *newClassType = alloc.new_object<MirType>(MirTypeKind::Class,
                                                      assignedId,
                                                      maxAlignmentInBytes,
                                                      totalSizeInBits,
                                                      lookupName,
                                                      std::move(fieldTypes));

    m_typeNames[lookupName] = newClassType;
    m_idToType[assignedId] = newClassType;

    return newClassType;
}

MirType *MirTypeTable::getFuncType(MirType *returnType,
                                   const std::pmr::list<MirRegister *> &parameters,
                                   const std::string_view &funcName)
{
    if (!returnType)
    {
        return nullptr;
    }

    auto structuralSignature = returnType->getName();
    structuralSignature += "(";

    std::pmr::vector<MirType *> subTypes(m_arena);
    subTypes.reserve(parameters.size() + 1);
    subTypes.push_back(returnType);

    for (auto &param : parameters)
    {
        if (!param)
            continue;
        MirType *paramType = param->getMirType();
        subTypes.push_back(paramType);

        if (subTypes.size() > 2)
        {
            structuralSignature += ",";
        }
        structuralSignature += paramType->getName();
    }
    structuralSignature += ")";

    std::pmr::string lookupKey(structuralSignature, m_arena);
    auto it = m_typeNames.find(lookupKey);
    if (it != m_typeNames.end())
    {
        return it->second;
    }

    size_t pointerSizeInBits = m_typeLayout->getPointerSizeInBytes() * 8;
    return create(MirTypeKind::Function, pointerSizeInBits, std::move(subTypes), lookupKey);
}

MirType *MirTypeTable::getPtr(MirType *srcType)
{
    if (!srcType)
        return nullptr;

    auto it = m_pointerCache.find(srcType);
    if (it != m_pointerCache.end())
    {
        return it->second;
    }

    size_t pointerSizeInBytes = m_typeLayout->getPointerSizeInBytes();
    std::pmr::vector<MirType *> childTarget({ srcType }, m_arena);
    std::string formattedName = std::format("{}*", srcType->getName());

    MirType *newPointerType =
            create(MirTypeKind::Pointer, pointerSizeInBytes * 8, std::move(childTarget), formattedName);

    m_pointerCache[srcType] = newPointerType;
    return newPointerType;
}

MirType *MirTypeTable::getArray(MirType *elementType, size_t elementCount)
{
    if (!elementType)
        return nullptr;

    std::string arraySignature = std::format("{}[{}]", elementType->getName(), elementCount);
    std::pmr::string lookupName(arraySignature, m_arena);

    auto it = m_typeNames.find(lookupName);
    if (it != m_typeNames.end())
    {
        return it->second;
    }

    size_t totalSizeInBytes = m_typeLayout->getTypeSizeInBytes(elementType) * elementCount;
    size_t alignmentInBytes = m_typeLayout->getTypeAlignmentInBytes(elementType); // Arrays match element alignment
    std::pmr::vector<MirType *> childType({ elementType }, m_arena);

    std::pmr::polymorphic_allocator<MirType> alloc(m_arena);
    size_t assignedId = ++m_currentId;
    MirType *newArrayType = alloc.new_object<MirType>(MirTypeKind::Array,
                                                      assignedId,
                                                      alignmentInBytes,
                                                      totalSizeInBytes * 8,
                                                      lookupName,
                                                      std::move(childType));

    m_typeNames[lookupName] = newArrayType;
    m_idToType[assignedId] = newArrayType;

    return newArrayType;
}

MirType *MirTypeTable::getFloatingTypeBySize(size_t sizeInBits) const
{
    MirType *result = nullptr;
    for (auto &[id, type] : m_idToType)
    {
        if (type->getKind() != MirTypeKind::FloatingPoint)
            continue;

        size_t typeSize = type->getTotalSizeInBits();
        if (typeSize >= sizeInBits)
        {
            if (!result || typeSize < result->getTotalSizeInBits())
            {
                result = type;
            }
        }
    }
    return result;
}

MirType *MirTypeTable::getIntegerTypeBySize(size_t sizeInBits) const
{
    MirType *result = nullptr;
    for (auto &[id, type] : m_idToType)
    {
        if (type->getKind() != MirTypeKind::Integer)
            continue;

        size_t typeSize = type->getTotalSizeInBits();
        if (typeSize >= sizeInBits)
        {
            if (!result || typeSize < result->getTotalSizeInBits())
            {
                result = type;
            }
        }
    }
    return result;
}

MirType *MirTypeTable::getMirTypeById(size_t id) const
{
    auto it = m_idToType.find(id);
    return (it != m_idToType.end()) ? it->second : nullptr;
}

MirType *MirTypeTable::getVoidType() const { return m_voidType; }
MirType *MirTypeTable::i1() const { return m_int1Type; }
MirType *MirTypeTable::i8() const { return m_int8Type; }
MirType *MirTypeTable::i16() const { return m_int16Type; }
MirType *MirTypeTable::i32() const { return m_int32Type; }
MirType *MirTypeTable::i64() const { return m_int64Type; }
MirType *MirTypeTable::i128() const { return m_int128Type; }
MirType *MirTypeTable::i256() const { return m_int256Type; }

MirType *MirTypeTable::f32() const { return m_float32Type; }
MirType *MirTypeTable::f64() const { return m_float64Type; }
MirType *MirTypeTable::f128() const { return m_float128Type; }

void MirTypeTable::initialize()
{
    m_voidType = create(MirTypeKind::Void, 0, {}, "void");
    m_int1Type = create(MirTypeKind::Integer, 1, {}, "i1");
    m_int8Type = create(MirTypeKind::Integer, 8, {}, "i8");
    m_int16Type = create(MirTypeKind::Integer, 16, {}, "i16");
    m_int32Type = create(MirTypeKind::Integer, 32, {}, "i32");
    m_int64Type = create(MirTypeKind::Integer, 64, {}, "i64");
    m_int128Type = create(MirTypeKind::Integer, 128, {}, "i128");
    m_int256Type = create(MirTypeKind::Integer, 256, {}, "i256");

    m_float32Type = create(MirTypeKind::FloatingPoint, 32, {}, "f32");
    m_float64Type = create(MirTypeKind::FloatingPoint, 64, {}, "f64");
    m_float128Type = create(MirTypeKind::FloatingPoint, 128, {}, "f128");

    m_bindingToken = create(MirTypeKind::BindingToken, 0, std::pmr::vector<MirType *>{ m_arena }, "__bindToken");
}