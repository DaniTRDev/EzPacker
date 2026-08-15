#include "EzTripleTestSuite.h"

MirRegisterAllocator *EzTripleTestSuite::getRegisterAllocator() { return m_regAllocator.get(); }

void EzTripleTestSuite::create(const std::filesystem::path &workingPath)
{
    EzMirTestSuite::create(workingPath);
    m_regAllocator = std::make_shared<MirRegisterAllocator>();
}

void EzTripleTestSuite::destroy()
{
    m_regAllocator.reset();
    EzMirTestSuite::destroy();
}

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