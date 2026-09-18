#ifndef EZCOMPILERTESTSUITE_H
#define EZCOMPILERTESTSUITE_H

#include "gtest/gtest.h"
#include "EzCompilerCommon.h"
#include "CommandLineOptions.h"
#include "TargetTriple.h"
#include "DriverContext.h"
#include "TargetResolver.h"
#include "FrontendAdapter.h"
#include "CompilationPipeline.h"
#include "EmissionEngine.h"

class EzCompilerTestSuite : public ::testing::Test
{
  protected:
    void SetUp() override {}
    void TearDown() override {}
};

#endif // EZCOMPILERTESTSUITE_H
