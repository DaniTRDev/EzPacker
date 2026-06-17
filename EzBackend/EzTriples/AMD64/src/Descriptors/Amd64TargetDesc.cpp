#include "Descriptors/Amd64TargetDesc.h"

Amd64TargetDesc::Amd64TargetDesc(MirBuilderContext *ctx) : m_ctx(ctx) {}

const char *Amd64TargetDesc::getName() const { return "Amd64"; }

MirType *Amd64TargetDesc::getNearestLegalType(MirType *type)
{
    const auto &t = m_ctx->getTypeTable();

    if (type->getKind() == MirTypeKind::Integer)
    {
        if (type->getTotalSizeInBits() <= 8)
            return t->i8();

        if (type->getTotalSizeInBits() <= 16)
            return t->i16();

        if (type->getTotalSizeInBits() <= 32)
            return t->i32();

        /**
         * This ensure promotion for values between 33 and 64 bits and expansion for values bigger than 64 bits.
         */
        return t->i64();
    }
    else if (type->getKind() == MirTypeKind::FloatingPoint)
    {
        if (type->getTotalSizeInBits() <= 32)
            return t->f32();

        /**
         * This ensure promotion for values between 33 and 64 bits and expansion for values bigger than 64 bits.
         */
        return t->f64();
    }

    return nullptr;
}
