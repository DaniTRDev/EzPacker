#include "EzDslSemaTestSuite.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "SourceManager/SourceManager.h"

DiagnosticCollector *EzDslSemaTestSuite::getDiagCollector() { return m_diagnosticCollector; }

DiagnosticLogger *EzDslSemaTestSuite::getDiagLogger() { return m_diagnosticLogger; }

ParseContext EzDslSemaTestSuite::createParseContextFromBuff(const std::string &sourceName,
                                                            const std::string &sourceContent)
{
    size_t sourceId = m_sourceManager->addSourceContent(sourceName, sourceContent);
    if (sourceId == 0)
    {
        throw std::runtime_error("Failed to create ParseContext because source was already added");
    }

    return ParseContext(m_diagnosticCollector, m_sourceManager, sourceId, &m_allocator);
}

SourceManager *EzDslSemaTestSuite::getSourceManager() { return m_sourceManager; }

SymbolTable *EzDslSemaTestSuite::getSymbolTable() { return m_symbolTable; }

void EzDslSemaTestSuite::create()
{
    std::pmr::polymorphic_allocator<> alloc(&m_allocator);
    m_diagnosticCollector = alloc.new_object<DiagnosticCollector>();
    m_sourceManager = alloc.new_object<SourceManager>(std::filesystem::current_path(), &m_allocator);
    m_diagnosticLogger = alloc.new_object<DiagnosticLogger>(m_sourceManager);
    m_symbolTable = alloc.new_object<SymbolTable>(&m_allocator);

    m_diagnosticCollector->addListener(m_diagnosticLogger);
    m_diagnosticCollector->enableDiag(Diag_Trace);
    m_diagnosticCollector->enableDiag(Diag_Debug);
}

void EzDslSemaTestSuite::destroy()
{
    std::pmr::polymorphic_allocator<> alloc(&m_allocator);
    alloc.delete_object(m_symbolTable);
    alloc.delete_object(m_sourceManager);
    alloc.delete_object(m_diagnosticCollector);
    alloc.delete_object(m_diagnosticLogger);
    m_allocator.release();
}

void EzDslSemaTestSuite::registerType(std::string_view name,
                                      DSL::Ast::TypeDef::TypeKind kind,
                                      uint32_t bitWidth,
                                      uint32_t alignment,
                                      uint8_t compactId)
{
    Symbols::TypeSymbol typeSym{ .m_name = name,
                                 .m_kind = kind,
                                 .m_bitWidth = bitWidth,
                                 .m_alignment = alignment,
                                 .m_compactId = compactId };

    m_symbolTable->declareSym(nullptr, SymbolType::Type, std::move(typeSym), name);
}

std::pmr::memory_resource *EzDslSemaTestSuite::getAllocator() { return &m_allocator; }

void EzDslSemaTestSuiteAsGtest::SetUp()
{
    Test::SetUp();
    EzDslSemaTestSuite::create();
}

void EzDslSemaTestSuiteAsGtest::TearDown()
{
    EzDslSemaTestSuite::destroy();
    Test::TearDown();
}