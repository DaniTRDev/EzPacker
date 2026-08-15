#include "EzTripleTestSuite.h"

void EzTripleTestSuite::create(const std::filesystem::path &workingPath) { EzMirTestSuite::create(workingPath); }

void EzTripleTestSuite::destroy() { EzMirTestSuite::destroy(); }

void MirTripleTestSuiteAsGtest::SetUp()
{
    Test::SetUp();
    EzTripleTestSuite::create(std::filesystem::current_path());
}

void MirTripleTestSuiteAsGtest::TearDown()
{
    EzTripleTestSuite::destroy();
    Test::TearDown();
}