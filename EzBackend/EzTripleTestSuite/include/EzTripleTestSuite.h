#ifndef EZPACKER_EZTRIPLETESTSUITE_H
#define EZPACKER_EZTRIPLETESTSUITE_H

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
 * This class is used as a common entry point for triple tests. It uses the inherited target desc from EzMirTestSuite
 */
class EzTripleTestSuite : public EzMirTestSuite
{
  public:
    /**
     * Returns the emitter context.
     */
    CodeEmitterContext *getEmitterCtx();

    /**
     * Returns the code emitter for the test triple.
     */
    EzTestTripleCodeEmitter *getEmitter();

    /**
     * Returns the dissasembler for the test triple
     */
    EzTestTripleDisassembler *getDisassembler();

    /**
     * Returns the target binary descriptor used for the test suite.
     */
    TargetBinaryDesc *getTargetBinaryDesc();

    /**
     * Creates common pointers used in test cases. Also calls EzMirTestSuite::create.
     * @param workingPath
     */
    virtual void create(const std::filesystem::path &workingPath) override;

    /**
     * Frees everything of this test suite. Also calls EzMirTestSuite::destroy.
     */
    virtual void destroy() override;

  private:
    CodeEmitterContext *m_emitterCtx;
    EzTestTripleCodeEmitter *m_emitter;
    EzTestTripleDisassembler *m_disassembler;
    TargetBinaryDesc *m_targetBinaryDesc;
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
