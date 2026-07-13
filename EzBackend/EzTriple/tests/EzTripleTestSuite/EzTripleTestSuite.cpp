#include "EzTripleTestSuite.h"

TargetDesc *EzTripleTestSuite::getTargetDesc() const { return m_targetDesc.get(); }

MirLegalizer *EzTripleTestSuite::getLegalizer() const { return m_legalizer.get(); }

void EzTripleTestSuite::addRule(LegalizeAction *action,
                                MirInstructionOpCode opcode,
                                std::vector<size_t> expectedOperandTypes)
{
    EXPECT_NE(m_legalizer.get(), nullptr);
    m_legalizer->addRule(action, opcode, std::move(expectedOperandTypes));
}

void EzTripleTestSuite::addRuleForCategory(LegalizeAction *action,
                                           MirInstructionCategory category,
                                           std::vector<size_t> expectedOperandTypes)
{
    EXPECT_NE(m_legalizer.get(), nullptr);
    m_legalizer->addRuleForCategory(action, category, expectedOperandTypes);
}

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
    return std::make_shared<EzTripleTargetDesc>(getBuilderCtx());
}

std::shared_ptr<MirLegalizer> MirTripleTestSuiteAsGtest::createTargetLegalizer()
{
    return std::make_shared<MirLegalizer>(getBuilderCtx(), getTargetDesc());
}
