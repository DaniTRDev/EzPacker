#ifndef EZLINKERTESTSUITE_H
#define EZLINKERTESTSUITE_H

#include "gtest/gtest.h"
#include "EzLinkerCommon.h"
#include "SystemLinkerDetector.h"
#include "EzLinkerDriver.h"
#include "EzExceptionRuntime.h"
#include "Rtti/RttiDescriptor.h"

class EzLinkerTestSuite : public ::testing::Test
{
protected:
    void SetUp() override
    {
        __ez_runtime_reset_for_testing();
    }

    void TearDown() override
    {
        __ez_runtime_reset_for_testing();
    }
};

#endif // EZLINKERTESTSUITE_H
