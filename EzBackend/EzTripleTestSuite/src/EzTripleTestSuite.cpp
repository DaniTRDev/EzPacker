#include "../include/EzTripleTestSuite.h"

TargetDesc *EzTripleTestSuite::getTargetDesc() const { return m_targetDesc.get(); }

MirLegalizer *EzTripleTestSuite::getLegalizer() const { return m_legalizer.get(); }

MirInstructionSelector *EzTripleTestSuite::getInstrSelector() const { return m_instructionSelector.get(); }

MirRegisterAllocator *EzTripleTestSuite::getRegisterAllocator() const { return m_registerAllocator.get(); }

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
    m_instructionSelector = createInstructionSelector();
    m_registerAllocator = createRegisterAllocator();

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

std::shared_ptr<MirRegisterAllocator> MirTripleTestSuiteAsGtest::createRegisterAllocator()
{
    return std::make_shared<MirRegisterAllocator>();
}

std::shared_ptr<MirInstructionSelector> MirTripleTestSuiteAsGtest::createInstructionSelector()
{
    std::shared_ptr<MirInstructionSelector> selector = std::make_shared<MirInstructionSelector>(getBuilderCtx());
    EzTripleTestSelector::create(getBuilderCtx(), selector.get());

    return selector;
}

std::shared_ptr<MirLegalizer> MirTripleTestSuiteAsGtest::createTargetLegalizer()
{
    std::shared_ptr<MirLegalizer> legalizer = std::make_shared<MirLegalizer>(getBuilderCtx());
    EzTripleTestLegalizer::create(getBuilderCtx(), legalizer.get());

    return legalizer;
}
