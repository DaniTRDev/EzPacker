#include "EzTripleTestSuite.h"

TargetDesc *EzTripleTestSuite::getTargetDesc() const { return m_targetDesc.get(); }

MirLegalizer *EzTripleTestSuite::getLegalizer() const { return m_legalizer.get(); }

void EzTripleTestSuite::create(const std::filesystem::path &workingPath) { EzMirTestSuite::create(workingPath); }

void EzTripleTestSuite::destroy()
{
    m_legalizer.reset();
    EzMirTestSuite::destroy();
}

void MirTripleTestSuiteAsGtest::SetUp()
{
    Test::SetUp();
    EzTripleTestSuite::create(std::filesystem::current_path());

    m_targetDesc = createTargetDesc();
    m_legalizer = createTargetLegalizer();

    getBuilderCtx()->getDiagCollector()->builder(Diag_Debug, "EzTripleTestSuite")
            << std::format("Using target descriptor: {}", m_targetDesc->getName()).c_str();
}

void MirTripleTestSuiteAsGtest::TearDown()
{
    EzTripleTestSuite::destroy();
    Test::TearDown();
}

std::shared_ptr<TargetDesc> MirTripleTestSuiteAsGtest::createTargetDesc()
{
    return std::make_shared<EzTripleTestTargetDesc>(getBuilderCtx());
}

std::shared_ptr<MirLegalizer> MirTripleTestSuiteAsGtest::createTargetLegalizer()
{
    std::shared_ptr<MirLegalizer> legalizer = std::make_shared<MirLegalizer>(getBuilderCtx(), getTargetDesc());
    EzTripleTestLegalizer::create(getBuilderCtx(), legalizer.get());

    return legalizer;
}
