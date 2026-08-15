#include "EzTripleTestSuite.h"

TargetBinaryDesc *EzTripleTestSuite::getTargetBinaryDesc() { return m_targetBinaryDesc; }

void EzTripleTestSuite::create(const std::filesystem::path &workingPath)
{
    EzMirTestSuite::create(workingPath);
    m_targetBinaryDesc = getTargetDesc()->getAvailableBinaryDescriptors().front();

    getBuilderCtx()->getDiagCollector()->builder(Diag_Debug, "EzMirTestSuite")
            << "Using binary descriptor: " << m_targetBinaryDesc->getName();
}

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