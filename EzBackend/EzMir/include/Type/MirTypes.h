#ifndef EZPACKER_MIRTYPES_H
#define EZPACKER_MIRTYPES_H

#include "Emitter/MirEmitterContext.h"

/**
 * Class used to store the least minimum required types so everything else works.
 */
class MirTypes
{
  public:
    MirTypes() = default;
    ~MirTypes() = default;

    MirType *getVoidType() const { return m_voidType; }

    MirType *getInt1Type() const { return m_int1Type; }
    MirType *getInt8Type() const { return m_int8Type; }
    MirType *getInt16Type() const { return m_int16Type; }
    MirType *getInt32Type() const { return m_int32Type; }
    MirType *getInt64Type() const { return m_int64Type; }

    MirType *getFloat32Type() const { return m_float32Type; }
    MirType *getFloat64Type() const { return m_float64Type; }

    /**
     * Returns a pointer to the given MirType
     * @param srcType
     * @return
     */
    MirType *getPtr(MirType *srcType)
    {
        // Pointers don't have pre adjusted size, depends on architecture.
        auto *subTypes = m_ctx->getTypePool()->createLinkedList<MirType>();
        m_ctx->getTypePool()->appendToListBack(subTypes, srcType);

        return m_ctx->createType(MirTypeKind::Pointer, 0, subTypes, std::format("{}*", srcType->getName()));
    }

    /**
     * Initializes the types in the context.
     * @param ctx
     */
    void initialize(MirEmitterContext *ctx)
    {
        m_ctx = ctx;

        m_voidType = m_ctx->createType(MirTypeKind::Void, 0, nullptr, "void");

        m_int1Type = m_ctx->createType(MirTypeKind::Integer, 1, nullptr, "i1");
        m_int8Type = m_ctx->createType(MirTypeKind::Integer, 1, nullptr, "i8");
        m_int16Type = m_ctx->createType(MirTypeKind::Integer, 2, nullptr, "i16");
        m_int32Type = m_ctx->createType(MirTypeKind::Integer, 4, nullptr, "i32");
        m_int64Type = m_ctx->createType(MirTypeKind::Integer, 8, nullptr, "i64");

        m_float32Type = m_ctx->createType(MirTypeKind::FloatingPoint, 4, nullptr, "f32");
        m_float64Type = m_ctx->createType(MirTypeKind::FloatingPoint, 8, nullptr, "f64");
    }

  private:
    MirEmitterContext *m_ctx;

    MirType *m_voidType;

    MirType *m_int1Type;
    MirType *m_int8Type;
    MirType *m_int16Type;
    MirType *m_int32Type;
    MirType *m_int64Type;

    MirType *m_float32Type;
    MirType *m_float64Type;
};

#endif // EZPACKER_MIRTYPES_H
