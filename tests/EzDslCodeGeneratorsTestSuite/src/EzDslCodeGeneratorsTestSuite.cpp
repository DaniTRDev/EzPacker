#include "EzDslCodeGeneratorsTestSuite.h"
#include "Ast/IrInstructionDefLangAst.h"
#include "Ast/TypeDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Parser/IrInstructionDefLang.h"
#include "Parser/TypeDefLang.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "SourceManager/SourceManager.h"

#include <format>
#include <fstream>
#include <sstream>

DiagnosticCollector *EzDslCodeGeneratorsTestSuite::getDiagCollector()
{
    return m_diagnosticCollector;
}

DiagnosticLogger *EzDslCodeGeneratorsTestSuite::getDiagLogger()
{
    return m_diagnosticLogger;
}

ParseContext EzDslCodeGeneratorsTestSuite::createParseContextFromBuff(const std::string &sourceName,
                                                                      const std::string &sourceContent)
{
    size_t sourceId = m_sourceManager->addSourceContent(sourceName, sourceContent);
    if (sourceId == 0)
    {
        throw std::runtime_error("Failed to create ParseContext because source was already added: " + sourceName);
    }

    return ParseContext(m_diagnosticCollector, m_sourceManager, sourceId, &m_allocator);
}

SourceManager *EzDslCodeGeneratorsTestSuite::getSourceManager()
{
    return m_sourceManager;
}

SymbolTable *EzDslCodeGeneratorsTestSuite::getSymbolTable()
{
    return m_symbolTable;
}

void EzDslCodeGeneratorsTestSuite::create()
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

void EzDslCodeGeneratorsTestSuite::destroy()
{
    std::pmr::polymorphic_allocator<> alloc(&m_allocator);
    alloc.delete_object(m_symbolTable);
    alloc.delete_object(m_sourceManager);
    alloc.delete_object(m_diagnosticCollector);
    alloc.delete_object(m_diagnosticLogger);
    m_allocator.release();
}

void EzDslCodeGeneratorsTestSuite::registerType(std::string_view name,
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

std::optional<DSL::Ast::IrInstDef::IrInstDefFile> EzDslCodeGeneratorsTestSuite::parseIrInstDefFile(
        const std::string &sourceContent)
{
    ParseContext ctx = createParseContextFromBuff(std::format("gen_test_{}.irdf", ++m_sourceCounter), sourceContent);
    return ctx.parse<DSL::Parser::IrInstDef::IrInstDefFile, DSL::Ast::IrInstDef::IrInstDefFile>();
}

std::optional<DSL::Ast::TypeDef::TypeDefFile> EzDslCodeGeneratorsTestSuite::parseTypeDefFile(
        const std::string &sourceContent)
{
    ParseContext ctx = createParseContextFromBuff(std::format("gen_test_{}.tyf", ++m_sourceCounter), sourceContent);
    return ctx.parse<DSL::Parser::TypeDef::TypeDefFile, DSL::Ast::TypeDef::TypeDefFile>();
}

std::string EzDslCodeGeneratorsTestSuite::readFileContent(const std::filesystem::path &filePath)
{
    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        return {};
    }
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

std::pmr::memory_resource *EzDslCodeGeneratorsTestSuite::getAllocator()
{
    return &m_allocator;
}

void EzDslCodeGeneratorsTestSuiteAsGtest::SetUp()
{
    Test::SetUp();
    EzDslCodeGeneratorsTestSuite::create();
    m_testTempDir = std::filesystem::temp_directory_path() / std::format("ezdsl_codegen_test_{}", ++s_testCounter);
    std::filesystem::create_directories(m_testTempDir);
}

void EzDslCodeGeneratorsTestSuiteAsGtest::TearDown()
{
    std::error_code ec;
    std::filesystem::remove_all(m_testTempDir, ec);
    EzDslCodeGeneratorsTestSuite::destroy();
    Test::TearDown();
}
