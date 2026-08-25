#ifndef EZMIRTESTSUITE_EZ_MIR_TEST_SUITE_H
#define EZMIRTESTSUITE_EZ_MIR_TEST_SUITE_H

#include "gtest/gtest.h"
#include "Instruction/MirInstructionBuilder.h"
#include "MirPasses/MirPassManager.h"

class DiagnosticCollector;
class DiagnosticLogger;
class EzMirTestSuiteCallingConv;
class EzMirTestSuiteTypeLayout;
class FlexInt;
class MirBuilderContext;
class MirFunction;
class MirInstruction;
class MirInstructionInsertionPoint;
class MirPass;
class MirPassManager;
class MirType;
class MirTypeTable;
class SourceManager;

enum class MirInstructionOpCode : uint16_t;

/**
 * Common test harness and helper container for EzMir unit tests.
 * Provides pre-initialized MIR building blocks including memory arenas,
 * diagnostic collectors, type tables, pass managers, and a default TEST function.
 */
class EzMirTestSuite
{
  public:
    /**
     * Returns the builder context used across MIR test cases.
     */
    MirBuilderContext *getBuilderCtx();

    /**
     * Returns the pre-constructed synthetic TEST function.
     */
    MirFunction *getTestFunc();

    /**
     * Returns the MIR pass manager linked to this test instance.
     */
    MirPassManager *getPassManager();

    /**
     * Returns the insertion point located at the beginning of the TEST function's entry block.
     */
    const MirInstructionInsertionPoint &getTestInsertionPoint();

    /**
     * Registers and executes a compiler pass of type PassType with the provided arguments,
     * executing it immediately on the test suite's builder context in test mode.
     */
    template <typename PassType, typename... Args>
        requires(std::is_base_of<MirPass, PassType>::value)
    PassType *runPass(Args &&...args)
    {
        MirPassManager *passManager = getPassManager();
        PassType *pass = (PassType *)passManager->addPass<PassType, Args...>(std::forward<Args>(args)...);

        passManager->runPass(pass, m_builderCtx.get());
        return pass;
    }

    /**
     * Returns the MIR type table holding primitive and target type layout definitions.
     */
    MirTypeTable *getTypeTable();

    /**
     * Inserts a two-operand register-to-register instruction at the current test insertion point.
     * Creates virtual destination and source registers with the given MIR types.
     */
    MirInstruction *addTestInstructionRegReg(MirInstructionOpCode opcode, MirType *destOperType, MirType *srcOperType);

    /**
     * Inserts a register-to-integer-immediate instruction at the current test insertion point.
     * Creates a virtual destination register and an integer immediate operand with the specified value.
     */
    MirInstruction *addTestInstructionRegIntImm(MirInstructionOpCode opcode,
                                                MirType *destOperType,
                                                MirType *srcOperType,
                                                FlexInt srcValue);

    /**
     * Inserts a register-to-float-immediate instruction at the current test insertion point.
     * Creates a virtual destination register and a 32-bit floating point immediate operand.
     */
    MirInstruction *addTestInstructionRegFloatImm(MirInstructionOpCode opcode, MirType *destOperType, float srcValue);

    /**
     * Inserts a register-to-memory instruction at the current test insertion point.
     * Creates a virtual destination register and a memory operand with base virtual register and displacement.
     */
    MirInstruction *addTestInstructionRegMem(MirInstructionOpCode opcode,
                                             MirType *destOperType,
                                             MirType *srcOperType,
                                             const FlexInt &displacement);

    /**
     * Initializes all diagnostic infrastructure, type tables, target layout,
     * default calling convention, and the synthetic void TEST function with an entry block.
     */
    virtual void create(const std::filesystem::path &workingPath);

    /**
     * Cleans up and releases all allocated MIR contexts and diagnostic structures.
     */
    virtual void destroy();

    /**
     * Retrieves the intrusive list of MIR functions registered in the current builder context.
     */
    IntrusiveLinkedList<MirFunction> &getFunctions();

  private:
    MirFunction *m_testFunction; // Pre-created function used to be able to create quick tests easily.
    MirInstructionInsertionPoint m_insertPoint;
    std::shared_ptr<EzMirTestSuiteCallingConv> m_callingConv;
    std::shared_ptr<EzMirTestSuiteTypeLayout> m_typeLayout;
    std::pmr::monotonic_buffer_resource m_arena;
    std::shared_ptr<DiagnosticCollector> m_diagCollector;
    std::shared_ptr<DiagnosticLogger> m_diagLogger;
    std::shared_ptr<MirBuilderContext> m_builderCtx;
    std::shared_ptr<MirPassManager> m_passManager;
    std::shared_ptr<MirTypeTable> m_typeTable;
    std::shared_ptr<SourceManager> m_sourceManager;
};

/**
 * GoogleTest test fixture adapter that invokes EzMirTestSuite::create
 * in SetUp() and EzMirTestSuite::destroy in TearDown().
 */
class MirTestSuiteAsGtest : public EzMirTestSuite, public ::testing::Test
{
  public:
    /**
     * Sets up the test environment by creating the EzMirTestSuite context.
     */
    void SetUp() override;

    /**
     * Tears down the test environment by freeing the EzMirTestSuite context.
     */
    void TearDown() override;

  private:
};

#endif // EZPACKER_EZMIRTESTSUITE_H