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

void EzTripleTestSuite::create(const std::filesystem::path &workingPath)
{
    EzMirTestSuite::create(workingPath);

    if (!m_targetDesc)
    {
        m_targetDesc = std::make_shared<EzTripleTargetDesc>(getBuilderCtx());

        getBuilderCtx()->getDiagCollector()->builder(Diag_Debug, "EzTripleTestSuite")
                << std::format("Using default target descriptor: {}", m_targetDesc->getName()).c_str();
    }

    m_legalizer = std::make_shared<MirLegalizer>(getBuilderCtx(), m_targetDesc.get());
}

void EzTripleTestSuite::destroy()
{
    m_legalizer.reset();
    EzMirTestSuite::destroy();
}

void EzTripleTestSuite::setTargetDesc(TargetDesc *desc) { m_targetDesc = std::shared_ptr<TargetDesc>(desc); }

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
