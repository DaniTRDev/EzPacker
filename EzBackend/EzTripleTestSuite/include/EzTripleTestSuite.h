#ifndef EZPACKER_EZTRIPLETESTSUITE_H
#define EZPACKER_EZTRIPLETESTSUITE_H

#include "EzTriple.h"
#include "EzMirTestSuite.h"
#include "EzTripleTestTargetDescriptor.h"
#include "EzTripleTestLegalizer.h"
#include "EzTripleTestSelector.h"
#include "Verifiers/CallAbiLowererVerifier.h"
#include "Verifiers/ExpandScalarActionVerifier.h"
#include "Verifiers/FrameLowererPassVerifier.h"
#include "Verifiers/FuncSignaturePassVerifier.h"
#include "Verifiers/FunctionParametersAbiLowererVerifier.h"
#include "Verifiers/InstructionSelectorPassVerifier.h"
#include "Verifiers/LegalizeCallActionVerifier.h"
#include "Verifiers/LegalizeReturnActionVerifier.h"
#include "Verifiers/PromoteScalarActionVerifier.h"
#include "Verifiers/RegisterAllocatorPassVerifier.h"
#include "Verifiers/ReturnAbiLowererVerifier.h"

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
     * @return MirLegalizer*
     */
    MirLegalizer *getLegalizer() const;

    /**
     * Returns a pointer to the instruction selector of the selected triple.
     * @return MirInstructionSelector*
     */
    MirInstructionSelector *getInstrSelector() const;

    /**
     * Returns a pointer to the register allocator of the selected triple.
     */
    MirRegisterAllocator *getRegisterAllocator() const;

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
     * Creates a target instruction selector and returns it.
     * @return
     */
    virtual std::shared_ptr<MirInstructionSelector> createInstructionSelector() = 0;

    /**
     * Creates a target description and returns it.
     * @return
     */
    virtual std::shared_ptr<TargetDesc> createTargetDesc() = 0;

    /**
     * Creates a register allocator and returns it.
     * @return
     */
    virtual std::shared_ptr<MirRegisterAllocator> createRegisterAllocator() = 0;

  protected:
    std::shared_ptr<TargetDesc> m_targetDesc;
    std::shared_ptr<MirLegalizer> m_legalizer;
    std::shared_ptr<MirInstructionSelector> m_instructionSelector;
    std::shared_ptr<MirRegisterAllocator> m_registerAllocator;
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
     * Creates a default MirLegalizer (EzTripleTestLegalizer) and returns it. This call must be done after the target
     * description has already been created. Parent classes can still override this method to inject their own target
     * description.
     * @return
     */
    virtual std::shared_ptr<MirLegalizer> createTargetLegalizer() override;

    /**
     * Creates a default MirInstructionSelector (EzTripleTestSelector) and returns it.
     * @return
     */
    virtual std::shared_ptr<MirInstructionSelector> createInstructionSelector() override;

    /**
     * Creates a target EzTripleTestTargetDesc and returns it. Parent classes can still override this method to inject
     * their own target description.
     * @return
     */
    virtual std::shared_ptr<TargetDesc> createTargetDesc() override;

    /**
     * Creates a default MirRegisterAllocator and returns it.
     */
    virtual std::shared_ptr<MirRegisterAllocator> createRegisterAllocator() override;
};

#endif // EZPACKER_EZTRIPLETESTSUITE_H
