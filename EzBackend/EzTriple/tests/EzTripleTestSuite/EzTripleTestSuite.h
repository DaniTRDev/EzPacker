#ifndef EZPACKER_EZTRIPLETESTSUITE_H
#define EZPACKER_EZTRIPLETESTSUITE_H

#include "EzTriple.h"
#include "EzMirTestSuite.h"
#include "EzTripleTestTargetDescriptor.h"
#include "ExpandScalarActionVerifier.h"
#include "FuncSignaturePassVerifier.h"
#include "LegalizeCallActionVerifier.h"
#include "LegalizeReturnActionVerifier.h"
#include "PromoteScalarActionVerifier.h"

/**
 * This class is used as a common entry point for triple tests. It takes 2 template parameters to be able to define
 * the current architecture we are compiling in.
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
     * Creates a target legalizer and returns it. This call must be done after the target description has already been
     * created.
     * @return
     */
    virtual std::shared_ptr<MirLegalizer> createTargetLegalizer() = 0;

    /**
     * Creates a target description and returns it.
     * @return
     */
    virtual std::shared_ptr<TargetDesc> createTargetDesc() = 0;

  protected:
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

    /**
     * Creates a default-empty MirLegalizer and returns it. This call must be done after the target description has
     * already been created. Parent classes can still override this method to inject
     * their own target description.
     * @return
     */
    virtual std::shared_ptr<MirLegalizer> createTargetLegalizer() override;

    /**
     * Creates a target EzTripleTestTargetDesc and returns it. Parent classes can still override this method to inject
     * their own target description.
     * @return
     */
    virtual std::shared_ptr<TargetDesc> createTargetDesc() override;
};

#endif // EZPACKER_EZTRIPLETESTSUITE_H
