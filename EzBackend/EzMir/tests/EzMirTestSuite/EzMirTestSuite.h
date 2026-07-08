#ifndef EZPACKER_EZMIRTESTSUITE_H
#define EZPACKER_EZMIRTESTSUITE_H

#include "Verifiers/MirCoreVerifiers.h"
#include "Verifiers/CodeFlowPassVerifier.h"
#include "Verifiers/LivenessPassVerifier.h"

/**
 * Class used to contain helper methods related to creation/destruction of needed objects in common test scenarios.
 */
class EzMirTestSuite
{
  public:
    /**
     * Returns the builder context used by this test.
     * @return
     */
    MirBuilderContext *getBuilderCtx();

    /**
     * Returns the TEST function.
     * @return
     */
    MirFunction *getTestFunc();

    /**
     * Returns the pass manager linked to this test.
     * @return
     */
    MirPassManager *getPassManager();

    /**
     * Returns the insertion point of the first block of the TEST function.
     * @return
     */
    const MirInstructionInsertionPoint &getTestInsertionPoint();

    /**
     * Runs the given pass.
     * @param pass
     * @return
     */
    template <typename PassType, typename... Args>
        requires(std::is_base_of<MirPass, PassType>::value)
    PassType *runPass(Args &&...args)
    {
        MirPassManager *passManager = getPassManager();
        PassType *pass = (PassType *)passManager->addPass<PassType, Args...>(std::forward<Args>(args)...);

        if constexpr (std::is_base_of<IMirAnalysisPass, PassType>::value)
        {
            // We can't use passManager->getAnalysis due to templates needed to be resolved at compile time.
            pass = passManager->getAnalysis<PassType>(getFunctions());
        }
        else
        {
            passManager->generatePipeline();
            passManager->runPipeline(getFunctions());
        }

        return pass;
    }

    /**
     * Returns a printer linked to the test context.
     * @return
     */
    MirPrinter getPrinter();

    /**
     * Returns the type table.
     * @return
     */
    MirTypeTable *getTypeTable();

    /**
     * Adds a test instruction USING THE CURRENT INSERTION POINT, of the form register-register.
     * @param destOperType
     * @param srcOperType
     */
    void addTestInstructionRegReg(MirInstructionOpCode opcode, MirType *destOperType, MirType *srcOperType);

    /**
     * Adds a test instruction USING THE CURRENT INSERTION POINT, of the form register-immediate(int).
     * @param opcode
     * @param destOperType
     * @param srcOperType
     * @param srcValue
     */
    void addTestInstructionRegIntImm(MirInstructionOpCode opcode,
                                     MirType *destOperType,
                                     MirType *srcOperType,
                                     int64_t srcValue);

    /**
     * Adds a test instruction USING THE CURRENT INSERTION POINT, of the form register-immediate(float).
     * @param opcode
     * @param destOperType
     * @param srcOperType
     * @param srcValue
     */
    void addTestInstructionRegFloatImm(MirInstructionOpCode opcode, MirType *destOperType, float srcValue);

    /**
     * Creates all the needed context pointers in a basic state for a test. It also creates 1 void "TEST" function,
     * without parameters. Might be overriden by parent classes, but THEY MUST CALL THIS METHOD.
     * @param workingPath
     */
    virtual void create(const std::filesystem::path &workingPath);

    /**
     * Frees everything of this test suite. Might be overriden by parent classes, but THEY MUST CALL THIS METHOD.
     */
    virtual void destroy();

    /**
     * Returns the MUTABLE function list.
     * @return
     */
    std::pmr::list<MirFunction *> &getFunctions();

  private:
    MirFunction *m_testFunction; // Pre-created function used to be able to create quick tests easily.
    MirInstructionInsertionPoint m_insertPoint;
    std::pmr::monotonic_buffer_resource m_arena;
    std::shared_ptr<DiagnosticCollector> m_diagCollector;
    std::shared_ptr<DiagnosticLogger> m_diagLogger;
    std::shared_ptr<MirBuilderContext> m_builderCtx;
    std::shared_ptr<MirPassManager> m_passManager;
    std::shared_ptr<MirTypeTable> m_typeTable;
    std::shared_ptr<SourceManager> m_sourceManager;
};

class MirTestSuiteAsGtest : public EzMirTestSuite, public ::testing::Test
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

  private:
};

#endif // EZPACKER_EZMIRTESTSUITE_H
