#ifndef EZMIRTESTSUITE_EZ_MIR_TEST_SUITE_TYPE_LAYOUT
#define EZMIRTESTSUITE_EZ_MIR_TEST_SUITE_TYPE_LAYOUT

#include "Type/IMirTargetTypeLayout.h"

class EzMirTestSuiteTypeLayout : public IMirTargetTypeLayout
{
  public:
    size_t getPointerSizeInBytes() const override;

    size_t getTypeAlignmentInBytes(const class MirType *type) const override;

    size_t getTypeSizeInBytes(const class MirType *type) const override;
};

#endif // EZMIRTESTSUITE_EZ_MIR_TEST_SUITE_TYPE_LAYOUT