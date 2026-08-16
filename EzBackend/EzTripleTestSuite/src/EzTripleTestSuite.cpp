#include "EzTripleTestSuite.h"

CodeEmitterContext *EzTripleTestSuite::getEmitterCtx() { return m_emitterCtx; }

EzTestTripleCodeEmitter *EzTripleTestSuite::getEmitter() { return m_emitter; }

EzTestTripleDisassembler *EzTripleTestSuite::getDisassembler() { return m_disassembler; }

TargetBinaryDesc *EzTripleTestSuite::getTargetBinaryDesc() { return m_targetBinaryDesc; }

void EzTripleTestSuite::create(const std::filesystem::path &workingPath)
{
    EzMirTestSuite::create(workingPath);
    m_targetBinaryDesc = getTargetDesc()->getAvailableBinaryDescriptors().front();

    getBuilderCtx()->getDiagCollector()->builder(Diag_Debug, "EzMirTestSuite")
            << "Using binary descriptor: " << m_targetBinaryDesc->getName();

    std::pmr::memory_resource *rsc = getBuilderCtx()->getGlobalAllocator();
    std::pmr::polymorphic_allocator<> alloc(rsc);

    m_emitterCtx = alloc.new_object<CodeEmitterContext>(getBuilderCtx()->getDiagCollector().get(),
                                                        m_targetBinaryDesc->getSections(),
                                                        rsc);
    m_emitter = alloc.new_object<EzTestTripleCodeEmitter>();
    m_disassembler = alloc.new_object<EzTestTripleDisassembler>();
}

void EzTripleTestSuite::destroy() { EzMirTestSuite::destroy(); }

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