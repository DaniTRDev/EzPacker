#ifndef EZPACKER_EZTRIPLETESTSUITE_H
#define EZPACKER_EZTRIPLETESTSUITE_H

#include "EzTriple.h"
#include "EzMirTestSuite.h"
#include "EzTripleTargetDescriptor.h"
#include "ExpandScalarActionVerifier.h"
#include "PromoteScalarActionVerifier.h"

/**
 * This class is used as a common entry point for triple tests. It takes 2 template parameters to be able to define
 * the current architecture we are compiling in.
 *
 * IMPORTANT: For every tested case, the top-most test class MUST call setTargetDesc to set the descriptor of the
 * target. If the target desc is not set, the DEFAULT descriptor will be used (EzTripleTargetDescriptor.h).
 */
class EzTripleTestSuite : public EzMirTestSuite
{
  public:
    /**
     * Returns the target descriptor of the selected triple.
     * @return TargetDesc*
     */
    TargetDesc *getTargetDesc() const;

    /**
     * Returns a pointer to the legalizer of the selected triple.
     * @return TargetLegalizerType*
     */
    MirLegalizer *getLegalizer() const;

    /**
     * Adds a rule that executes an action on a match for the test legalizer. It internally uses MirLegalizer's method,
     * exposed for convenience.
     * @param action
     * @param opcode
     * @param expectedOperandTypes
     */
    void addRule(LegalizeAction *action, MirInstructionOpCode opcode, std::vector<size_t> expectedOperandTypes);

    /**
     * Adds a rule for EVERY INSTRUCTION inside the category. It internally uses MirLegalizer's method, exposed for
     * convenience.
     * @param action
     * @param category
     * @param expectedOperandTypes
     */
    void addRuleForCategory(LegalizeAction *action,
                            MirInstructionCategory category,
                            std::vector<size_t> expectedOperandTypes);

    /**
     * Creates common pointers used in test cases. Also calls EzMirTestSuite::create.
     * @param workingPath
     */
    virtual void create(const std::filesystem::path &workingPath) override;

    /**
     * Frees everything of this test suite. Also calls EzMirTestSuite::destroy.
     */
    virtual void destroy() override;

    /**
     * Sets the target descriptor for this test suite.
     * @param desc
     */
    void setTargetDesc(TargetDesc *desc);

  private:
    std::shared_ptr<TargetDesc> m_targetDesc;
    std::shared_ptr<MirLegalizer> m_legalizer;
};

class MirTripleTestSuiteAsGtest : public EzTripleTestSuite, public ::testing::Test
{
  public:
    /**
     * Calls EzMirTestSuite::create.
     */
    void SetUp() override;

    /**
     * Calls EzMirTestSuite::destroy.
     */
    void TearDown() override;
};

#endif // EZPACKER_EZTRIPLETESTSUITE_H
