#include "../include/EzMirTestSuite.h"

MirBuilderContext *EzMirTestSuite::getBuilderCtx() { return m_builderCtx.get(); }

MirFunction *EzMirTestSuite::getTestFunc() { return m_testFunction; }

const MirInstructionInsertionPoint &EzMirTestSuite::getTestInsertionPoint() { return m_insertPoint; }

MirPassManager *EzMirTestSuite::getPassManager() { return m_passManager.get(); }

MirTypeTable *EzMirTestSuite::getTypeTable() { return m_typeTable.get(); }

void EzMirTestSuite::create(const std::filesystem::path &workingPath)
{
    m_diagCollector = std::make_shared<DiagnosticCollector>();
    m_typeLayout = createTypeLayout();
    m_typeTable = std::make_shared<MirTypeTable>(m_typeLayout.get(), &m_arena);
    m_builderCtx = std::make_shared<MirBuilderContext>(nullptr, &m_arena, m_diagCollector, m_typeTable);
    m_sourceManager = std::make_shared<SourceManager>(workingPath);
    m_diagLogger = std::make_shared<DiagnosticLogger>(m_sourceManager.get());

    m_callingConv = createDefaultCallingConv();
    m_builderCtx->setDefaultCallingConvention(m_callingConv.get());

    m_passManager = std::make_shared<MirPassManager>(&m_arena, m_diagCollector);
    m_passManager->setTestMode();

    m_diagCollector->addListener(m_diagLogger.get());
    m_typeTable->initialize();
    m_testFunction = MirFunctionBuilder(m_builderCtx.get()).build(m_typeTable->getVoidType(), "TEST");

    m_diagCollector->builder(Diag_Debug, "EzMirTestSuite")
            << "Created pass manager in test mode (skip dependency resolution)";

    m_diagCollector->builder(Diag_Debug, "EzMirTestSuite")
            << "Using default calling convention: " << m_callingConv->getName();

    m_diagCollector->builder(Diag_Debug, "EzMirTestSuite") << "Adding common passes to pass manager";
    m_passManager->addPass<CodeFlowAnalysis>(getBuilderCtx());
    m_passManager->addPass<LivenessAnalysis>(getBuilderCtx());

    if (!m_testFunction)
    {
        m_diagCollector->builder(Diag_Error, "EzMirTestSuite") << "The creation of the test function failed!";
    }

    MirBlock *entryPoint = m_testFunction->getEntryPoint();

    m_insertPoint = { .m_type = InsertionType::InsertAfter,
                      .m_block = entryPoint,
                      .m_iterator = entryPoint->getInstructions().begin() };
}

void EzMirTestSuite::destroy()
{
    m_builderCtx.reset();
    m_typeTable.reset();
    m_sourceManager.reset();
    m_diagCollector.reset();
}

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

std::pmr::list<MirFunction *> &EzMirTestSuite::getFunctions() { return getBuilderCtx()->getFunctions(); }

std::shared_ptr<IMirTargetTypeLayout> EzMirTestSuite::createTypeLayout()
{
    return std::make_shared<TestTargetTypeLayout>();
}

std::shared_ptr<CallingConvDesc> EzMirTestSuite::createDefaultCallingConv()
{
    return std::make_shared<TestCallingConvention>(getBuilderCtx());
}

void MirTestSuiteAsGtest::SetUp()
{
    EzMirTestSuite::create(std::filesystem::current_path());
    Test::SetUp();
}

void MirTestSuiteAsGtest::TearDown()
{
    EzMirTestSuite::destroy();
    Test::TearDown();
}
