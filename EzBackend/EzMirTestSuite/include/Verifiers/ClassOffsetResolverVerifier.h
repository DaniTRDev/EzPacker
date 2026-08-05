#ifndef EZPACKER_CLASSOFFSETRESOLVERVERIFIER_H
#define EZPACKER_CLASSOFFSETRESOLVERVERIFIER_H

#include "MirCoreVerifiers.h"
#include "MirPasses/Passes/ClassOffsetResolverPass.h"

class ClassOffsetResolverVerifier : public MirPassVerifier<ClassOffsetResolverPass, ClassOffsetResolverVerifier>
{
  public:
    /**
     * Creates the verifier for the ClassOffsetResolverPass pass.
     * @param pass The executed pass object.
     * @param ctx The current compilation builder context.
     */
    ClassOffsetResolverVerifier(ClassOffsetResolverPass *pass, MirBuilderContext *ctx);

    /**
     * Verifies that the total size in bytes of a resolved class matches expectations.
     * @param classId Unique identifier of the class.
     * @param expectedSize Expected structural size in bytes including tail-padding.
     */
    ClassOffsetResolverVerifier &classSize(size_t classId, size_t expectedSize);

    /**
     * Verifies the maximum alignment multiple computed for a class structure layout.
     * @param classId Unique identifier of the class.
     * @param expectedAlign Expected max scalar element alignment bound.
     */
    ClassOffsetResolverVerifier &classAlignment(size_t classId, size_t expectedAlign);

    /**
     * Asserts that a class field has been resolved to a specific byte displacement offset.
     * @param classId Unique identifier of the class.
     * @param fieldName The name string identifier of the field.
     * @param expectedOffset The expected structural byte offset from the class base address pointer.
     */
    ClassOffsetResolverVerifier &fieldOffset(size_t classId, const std::string_view &fieldName, int64_t expectedOffset);

    /**
     * Asserts that a virtual method has been assigned a specific slot byte offset in the class VTable.
     * @param classId Unique identifier of the class.
     * @param slotIndex The zero-indexed position entry within the VTable sequence.
     * @param expectedOffset The expected indirect function pointer offset layout location.
     */
    ClassOffsetResolverVerifier &methodOffset(size_t classId, size_t slotIndex, int64_t expectedOffset);

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_CLASSOFFSETRESOLVERVERIFIER_H