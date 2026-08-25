#ifndef EZMIRTESTSUITE_EZ_MIR_TEST_SUITE_TYPE_LAYOUT
#define EZMIRTESTSUITE_EZ_MIR_TEST_SUITE_TYPE_LAYOUT

#include "Type/IMirTargetTypeLayout.h"

/**
 * Target Type Layout provider for the EzMir test suite.
 * Implements 64-bit target type sizing and alignment rules (e.g. 8-byte pointers,
 * standard power-of-two alignments for integers and floating-point types).
 */
class EzMirTestSuiteTypeLayout : public IMirTargetTypeLayout
{
  public:
    /**
     * Returns the target pointer size in bytes (8 bytes for 64-bit mock target).
     */
    size_t getPointerSizeInBytes() const override;

    /**
     * Calculates the memory alignment in bytes required for a given MIR type.
     */
    size_t getTypeAlignmentInBytes(const class MirType *type) const override;

    /**
     * Calculates the memory footprint size in bytes for a given MIR type.
     */
    size_t getTypeSizeInBytes(const class MirType *type) const override;
};

#endif // EZMIRTESTSUITE_EZ_MIR_TEST_SUITE_TYPE_LAYOUT