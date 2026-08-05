#include "../../include/Verifiers/ClassOffsetResolverVerifier.h"

ClassOffsetResolverVerifier::ClassOffsetResolverVerifier(ClassOffsetResolverPass *pass, MirBuilderContext *ctx) :
    MirPassVerifier(pass), m_ctx(ctx)
{
}

ClassOffsetResolverVerifier &ClassOffsetResolverVerifier::classSize(size_t classId, size_t expectedSize)
{
    MirClass *_class = m_ctx->getClassById(classId);
    EXPECT_NE(_class, nullptr) << std::format("Class ID {} not found in context", classId);
    if (_class)
    {
        EXPECT_EQ(_class->getType()->getTotalSizeInBytes(), expectedSize)
                << std::format("Class '{}' total size mismatch", _class->getName());
    }
    return *this;
}

ClassOffsetResolverVerifier &ClassOffsetResolverVerifier::classAlignment(size_t classId, size_t expectedAlign)
{
    MirClass *_class = m_ctx->getClassById(classId);
    EXPECT_NE(_class, nullptr) << std::format("Class ID {} not found in context", classId);
    if (_class)
    {
        EXPECT_EQ(_class->getType()->getMaxAlignmentInBytes(), expectedAlign)
                << std::format("Class '{}' maximum alignment mismatch", _class->getName());
    }
    return *this;
}

ClassOffsetResolverVerifier &
ClassOffsetResolverVerifier::fieldOffset(size_t classId, const std::string_view &fieldName, int64_t expectedOffset)
{
    MirClass *_class = m_ctx->getClassById(classId);
    EXPECT_NE(_class, nullptr) << std::format("Class ID {} not found in context", classId);
    if (_class)
    {
        MirClassField *field = _class->getFieldByName(fieldName);
        EXPECT_NE(field, nullptr) << std::format("Field '{}' not found in class '{}'", fieldName, _class->getName());
        if (field)
        {
            EXPECT_EQ(field->m_offset, expectedOffset)
                    << std::format("Field '{}' offset placement mismatch in class '{}'", fieldName, _class->getName());
        }
    }
    return *this;
}

ClassOffsetResolverVerifier &
ClassOffsetResolverVerifier::methodOffset(size_t classId, size_t slotIndex, int64_t expectedOffset)
{
    MirClass *_class = m_ctx->getClassById(classId);
    EXPECT_NE(_class, nullptr) << std::format("Class ID {} not found in context", classId);
    if (_class)
    {
        auto &vTable = _class->getVTable();
        EXPECT_LT(slotIndex, vTable.size())
                << std::format("VTable slot index {} out of bounds for class '{}'", slotIndex, _class->getName());
        if (slotIndex < vTable.size())
        {
            EXPECT_EQ(vTable[slotIndex]->m_offset, expectedOffset)
                    << std::format("VTable slot {} method offset mismatch in class '{}'", slotIndex, _class->getName());
        }
    }
    return *this;
}