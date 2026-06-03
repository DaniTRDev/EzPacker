#ifndef EZPACKER_MIRTYPETABLE_H
#define EZPACKER_MIRTYPETABLE_H

#include "Emitter/MirEmitterContext.h"

/**
 * Class used to store the least minimum required types so everything else works.
 */
class MirTypeTable
{
  public:
    MirTypeTable() = default;
    ~MirTypeTable() = default;

    MirType *create(MirTypeKind kind,
                    size_t totalSizeInBytes,
                    TypedPoolLinkedList<MirType> *subTypes,
                    const std::string_view &name)
    {
        if (name.empty())
        {
            emitError(ErrorSeverity::Fatal,
                      "Could not create type because name is empty",
                      "MirEmitterContext::createType");
            return nullptr;
        }

        if (m_typeNames.contains(name))
        {
            emitError(ErrorSeverity::Fatal,
                      "Could not create type because a type with the same name already exists",
                      "MirEmitterContext::createType");
            return nullptr;
        }

        if (!subTypes)
        {
            subTypes = m_typePool.linkedList<MirType>();
        }

        std::string_view copiedName = m_namePool.createConstantString(name.data());
        MirType *type = m_typePool.create<MirType>(kind, ++m_currentId, totalSizeInBytes, subTypes, copiedName.data());

        LOG_DEBUG(std::format("Creating type (id: {}) (name: {}) (size: {}) (subTypeCount: {})",
                              type->getId(),
                              type->getName(),
                              type->getTotalSizeInBytes(),
                              type->getSubTypes()->m_numElems),
                  "MirEmitterContext");

        m_typeNames.insert(copiedName);
        m_idToType.insert({ type->getId(), type });

        return type;
    }

    /**
     * Finds and returns an integer MirType of the specified size.
     * Returns nullptr if no such type has been created yet.
     */
    MirType *getIntegerTypeBySize(size_t sizeInBytes);
    
    /**
     * Returns a MirType out of the given ID.
     * @param id
     * @return
     */
    MirType *getMirTypeById(size_t id);
    
    /**
     * Returns a pointer to the given MirType
     * @param srcType
     * @return
     */
    MirType *getPtr(MirType *srcType)
    {
        // Pointers don't have pre adjusted size, depends on architecture.
        auto *subTypes = m_ctx->getTypePool()->linkedList<MirType>();
        subTypes->appendBack(srcType);

        return create(MirTypeKind::Pointer, 0, subTypes, std::format("{}*", srcType->getName()));
    }
    
    /**
     * Initializes the types in the context.
     * @param ctx
     */
    void initialize(MirEmitterContext *ctx)
    {
        m_ctx = ctx;

        m_voidType = create(MirTypeKind::Void, 0, nullptr, "void");

        m_int1Type = create(MirTypeKind::Integer, 1, nullptr, "i1");
        m_int8Type = create(MirTypeKind::Integer, 1, nullptr, "i8");
        m_int16Type = create(MirTypeKind::Integer, 2, nullptr, "i16");
        m_int32Type = create(MirTypeKind::Integer, 4, nullptr, "i32");
        m_int64Type = create(MirTypeKind::Integer, 8, nullptr, "i64");

        m_float32Type = create(MirTypeKind::FloatingPoint, 4, nullptr, "f32");
        m_float64Type = create(MirTypeKind::FloatingPoint, 8, nullptr, "f64");
    }
    
    MirType *getVoidType() const { return m_voidType; }
    
    MirType *getInt1Type() const { return m_int1Type; }
    MirType *getInt8Type() const { return m_int8Type; }
    MirType *getInt16Type() const { return m_int16Type; }
    MirType *getInt32Type() const { return m_int32Type; }
    MirType *getInt64Type() const { return m_int64Type; }

    MirType *getFloat32Type() const { return m_float32Type; }
    MirType *getFloat64Type() const { return m_float64Type; }

  private:
    MirEmitterContext *m_ctx;
    MirId m_currentId;

    MirType *m_voidType;

    MirType *m_int1Type;
    MirType *m_int8Type;
    MirType *m_int16Type;
    MirType *m_int32Type;
    MirType *m_int64Type;

    MirType *m_float32Type;
    MirType *m_float64Type;

    StringPool m_namePool;
    TypedPool m_typePool;
    std::set<std::string_view> m_typeNames;
    std::map<size_t, MirType *> m_idToType;
};

#endif // EZPACKER_MIRTYPETABLE_H
