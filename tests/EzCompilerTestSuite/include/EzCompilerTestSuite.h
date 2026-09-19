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

/**
 * Base test fixture for the EzCompiler test suite; provides no shared state
 * beyond the standard GoogleTest lifecycle hooks.
 */
class EzCompilerTestSuite : public ::testing::Test
{
  protected:
    // Per-test setup hook (no-op).
    void SetUp() override {}
    // Per-test teardown hook (no-op).
    void TearDown() override {}
};

#endif // EZCOMPILERTESTSUITE_H
