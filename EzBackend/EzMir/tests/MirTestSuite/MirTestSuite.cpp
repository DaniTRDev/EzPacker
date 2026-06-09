#include "MirTestSuite.h"

MirBuilderContext *MirTestSuite::getBuilderCtx() { return m_builderCtx.get(); }

MirFunction *MirTestSuite::getTestFunc() { return m_testFunction; }

MirInstructionInsertionPoint *MirTestSuite::getTestInsertionPoint() { return &m_insertPoint; }

MirPassManager *MirTestSuite::getPassManager() { return m_passManager.get(); }

MirPrinter MirTestSuite::getPrinter() { return MirPrinter(); }

MirTypeTable *MirTestSuite::getTypeTable() { return m_typeTable.get(); }

void MirTestSuite::create(const std::filesystem::path &workingPath)
{
    m_diagCollector = std::make_shared<DiagnosticCollector>();
    m_typeTable = std::make_shared<MirTypeTable>(&m_arena);
    m_builderCtx = std::make_shared<MirBuilderContext>(&m_arena, m_diagCollector, m_typeTable);
    m_sourceManager = std::make_shared<SourceManager>(workingPath);
    m_diagLogger = std::make_shared<DiagnosticLogger>(m_sourceManager.get());
    m_passManager = std::make_shared<MirPassManager>(&m_arena, m_diagCollector);

    m_diagCollector->addListener(m_diagLogger.get());
    m_typeTable->initialize();
    m_testFunction = MirFunctionBuilder(m_builderCtx.get()).build(m_typeTable->getVoidType(), nullptr, {}, "TEST");

    if (!m_testFunction)
    {
        m_diagCollector->builder(DiagnosticMessageType::Diag_Error, "MirTestSuite")
                << "The creation of the test function failed!";
    }

    MirBlock *entryPoint = m_testFunction->getEntryPoint();

    m_insertPoint = { .m_type = InsertionType::Append,
                      .m_block = entryPoint,
                      .m_iterator = entryPoint->getInstructions().begin() };
}

void MirTestSuite::destroy()
{
    m_builderCtx.reset();
    m_typeTable.reset();
    m_sourceManager.reset();
    m_diagCollector.reset();
}

std::pmr::list<MirFunction *> &MirTestSuite::getFunctions() { return m_builderCtx->getFunctions(); }

void MirTestSuiteAsGtest::SetUp()
{
    MirTestSuite::create(std::filesystem::current_path());
    Test::SetUp();
}

void MirTestSuiteAsGtest::TearDown()
{
    MirTestSuite::destroy();
    Test::TearDown();
}
