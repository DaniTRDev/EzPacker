#include "EzMirTestSuite.h"
#include "EzMirTestSuiteCallingConv.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "MirPasses/Passes/CodeFlowAnalysisPass.h"
#include "MirPasses/Passes/LivenessAnalysisPass.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "SourceManager/SourceManager.h"
#include "Type/MirTypeTable.h"

// Retrieves the MIR builder context associated with this test suite.
MirBuilderContext *EzMirTestSuite::getBuilderCtx() { return m_builderCtx.get(); }

// Retrieves the synthetic TEST function generated during test suite initialization.
MirFunction *EzMirTestSuite::getTestFunc() { return m_testFunction; }

// Retrieves the active instruction insertion point at the start of the TEST function's entry block.
const MirInstructionInsertionPoint &EzMirTestSuite::getTestInsertionPoint() { return m_insertPoint; }

// Retrieves the pass manager configured for test mode.
MirPassManager *EzMirTestSuite::getPassManager() { return m_passManager.get(); }

// Retrieves the MIR type table holding type layout and definitions.
MirTypeTable *EzMirTestSuite::getTypeTable() { return m_typeTable.get(); }

/**
 * Initializes all core compiler structures and test fixtures:
 * 1. Allocates diagnostic collector and logger attached to the current source manager.
 * 2. Initializes the MIR type table with a 64-bit mock target type layout.
 * 3. Creates the builder context and sets the mock calling convention.
 * 4. Configures the pass manager in test mode (bypassing strict dependency resolution).
 * 5. Constructs a default void "TEST" function with an entry block and sets the insertion point.
 */
void EzMirTestSuite::create(const std::filesystem::path &workingPath)
{
    m_diagCollector = std::make_shared<DiagnosticCollector>();
    m_typeTable = std::make_shared<MirTypeTable>(&m_arena);
    m_builderCtx = std::make_shared<MirBuilderContext>(nullptr, m_diagCollector.get(), m_typeTable.get(), &m_arena);
    m_sourceManager = std::make_shared<SourceManager>(workingPath, &m_arena);
    m_diagLogger = std::make_shared<DiagnosticLogger>(m_sourceManager.get());
    m_passManager = std::make_shared<MirPassManager>(m_diagCollector.get(), &m_arena);

    m_callingConv = std::make_shared<EzMirTestSuiteCallingConv>();
    m_diagCollector->addListener(m_diagLogger.get());
    m_diagCollector->enableDiag(Diag_Trace);
    m_diagCollector->enableDiag(Diag_Debug);

    m_typeTable->initialize(64);
    m_passManager->setTestMode();

    m_builderCtx->setDefaultCallingConvention(m_callingConv.get());

    m_testFunction = MirFunctionBuilder(m_builderCtx.get()).build(m_typeTable->_void(), {}, "TEST");

    m_diagCollector->builder(Diag_Debug, "EzMirTestSuite")
            << "Created pass manager in test mode (skip dependency resolution)";

    m_diagCollector->builder(Diag_Debug, "EzMirTestSuite")
            << "Using default calling convention: " << m_builderCtx->getDefaultCallingConvention()->getName();

    m_diagCollector->builder(Diag_Debug, "EzMirTestSuite") << "Adding common passes to pass manager";
    m_passManager->addPass<CodeFlowAnalysisPass>(getBuilderCtx());
    m_passManager->addPass<LivenessAnalysisPass>(getBuilderCtx());

    if (!m_testFunction)
    {
        m_diagCollector->builder(Diag_Error, "EzMirTestSuite") << "The creation of the test function failed!";
    }

    MirBlock *entryPoint = m_testFunction->getEntryPoint();

    m_insertPoint = { .m_type = InsertionType::InsertAfter,
                      .m_block = entryPoint,
                      .m_iterator = entryPoint->getInstructions().begin() };
}

/**
 * Releases all shared instances and frees resources allocated for the test fixture.
 */
void EzMirTestSuite::destroy()
{
    m_builderCtx.reset();
    m_typeTable.reset();
    m_sourceManager.reset();
    m_diagCollector.reset();
}

/**
 * Creates and inserts a binary register-to-register instruction (e.g. ADD %dest, %src)
 * at the current test insertion point with the provided operand types.
 */
MirInstruction *
EzMirTestSuite::addTestInstructionRegReg(MirInstructionOpCode opcode, MirType *destOperType, MirType *srcOperType)
{
    MirInstructionBuilder builder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder opBuilder(getBuilderCtx());

    return builder.build(
            opcode,
            nullptr,
            { opBuilder.buildVReg(destOperType, "testDest"), opBuilder.buildVReg(srcOperType, "testScr") });
}

/**
 * Creates and inserts a register-immediate instruction with an integer constant
 * at the current test insertion point.
 */
MirInstruction *EzMirTestSuite::addTestInstructionRegIntImm(MirInstructionOpCode opcode,
                                                            MirType *destOperType,
                                                            MirType *srcOperType,
                                                            FlexInt srcValue)
{
    MirInstructionBuilder builder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder opBuilder(getBuilderCtx());

    return builder.build(
            opcode,
            nullptr,
            { opBuilder.buildVReg(destOperType, "testDest"), opBuilder.buildInt(srcOperType, FlexInt(srcValue)) });
}

/**
 * Creates and inserts a register-immediate instruction with a 32-bit floating point constant
 * at the current test insertion point.
 */
MirInstruction *
EzMirTestSuite::addTestInstructionRegFloatImm(MirInstructionOpCode opcode, MirType *destOperType, float srcValue)
{
    MirInstructionBuilder builder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder opBuilder(getBuilderCtx());

    return builder.build(opcode,
                         nullptr,
                         { opBuilder.buildVReg(destOperType, "testDest"),
                           opBuilder.buildFloat(getTypeTable()->f32(), FlexFloat(srcValue)) });
}

/**
 * Creates and inserts a register-memory instruction with base virtual register and displacement
 * at the current test insertion point.
 */
MirInstruction *EzMirTestSuite::addTestInstructionRegMem(MirInstructionOpCode opcode,
                                                         MirType *destOperType,
                                                         MirType *srcOperType,
                                                         const FlexInt &displacement)
{
    MirInstructionBuilder builder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder opBuilder(getBuilderCtx());

    return builder.build(
            opcode,
            nullptr,
            { opBuilder.buildVReg(destOperType, "testDest"),
              opBuilder.buildMem(srcOperType, opBuilder.buildVReg(getTypeTable()->i64(), "testBase"), displacement) });
}

// Retrieves all functions registered in the active builder context.
IntrusiveLinkedList<MirFunction> &EzMirTestSuite::getFunctions() { return getBuilderCtx()->getFunctions(); }

// GoogleTest SetUp hook: initializes test suite using the current working directory.
void MirTestSuiteAsGtest::SetUp()
{
    EzMirTestSuite::create(std::filesystem::current_path());
    Test::SetUp();
}

// GoogleTest TearDown hook: cleans up test suite state and invokes base teardown.
void MirTestSuiteAsGtest::TearDown()
{
    EzMirTestSuite::destroy();
    Test::TearDown();
}
