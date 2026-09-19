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

// Retrieves the active diagnostic collector.
DiagnosticCollector *EzDslCodeGeneratorsTestSuite::getDiagCollector() { return m_diagnosticCollector; }

// Retrieves the diagnostic logger.
DiagnosticLogger *EzDslCodeGeneratorsTestSuite::getDiagLogger() { return m_diagnosticLogger; }

/**
 * Registers an in-memory source buffer with the source manager and returns a
 * ParseContext bound to the test allocator and diagnostics. Throws if the
 * source name was already registered.
 */
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

// Retrieves the source manager.
SourceManager *EzDslCodeGeneratorsTestSuite::getSourceManager() { return m_sourceManager; }

// Retrieves the symbol table.
SymbolTable *EzDslCodeGeneratorsTestSuite::getSymbolTable() { return m_symbolTable; }

/**
 * Allocates the diagnostic collector, logger, source manager, and symbol table
 * from the internal PMR buffer resource and enables trace/debug diagnostics.
 */
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

/**
 * Destroys all allocated generator test resources and releases the PMR buffer.
 */
void EzDslCodeGeneratorsTestSuite::destroy()
{
    std::pmr::polymorphic_allocator<> alloc(&m_allocator);
    alloc.delete_object(m_symbolTable);
    alloc.delete_object(m_sourceManager);
    alloc.delete_object(m_diagnosticCollector);
    alloc.delete_object(m_diagnosticLogger);
    m_allocator.release();
}

/**
 * Declares a type symbol with the given name, kind, bit width, alignment, and
 * compact id into the suite's symbol table.
 */
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

/**
 * Parses an IR instruction definition (.irdf) source string into its AST using
 * a uniquely named in-memory source buffer.
 */
std::optional<DSL::Ast::IrInstDef::IrInstDefFile>
EzDslCodeGeneratorsTestSuite::parseIrInstDefFile(const std::string &sourceContent)
{
    ParseContext ctx = createParseContextFromBuff(std::format("gen_test_{}.irdf", ++m_sourceCounter), sourceContent);
    return ctx.parse<DSL::Parser::IrInstDef::IrInstDefFile, DSL::Ast::IrInstDef::IrInstDefFile>();
}

/**
 * Parses a type definition (.tyf) source string into its AST using a uniquely
 * named in-memory source buffer.
 */
std::optional<DSL::Ast::TypeDef::TypeDefFile>
EzDslCodeGeneratorsTestSuite::parseTypeDefFile(const std::string &sourceContent)
{
    ParseContext ctx = createParseContextFromBuff(std::format("gen_test_{}.tyf", ++m_sourceCounter), sourceContent);
    return ctx.parse<DSL::Parser::TypeDef::TypeDefFile, DSL::Ast::TypeDef::TypeDefFile>();
}

/**
 * Reads the entire contents of a file into a string, returning an empty string
 * if the file cannot be opened.
 */
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

// Retrieves the monotonic memory resource.
std::pmr::memory_resource *EzDslCodeGeneratorsTestSuite::getAllocator() { return &m_allocator; }

// GoogleTest SetUp hook: initializes the suite and creates an isolated temp directory.
void EzDslCodeGeneratorsTestSuiteAsGtest::SetUp()
{
    Test::SetUp();
    EzDslCodeGeneratorsTestSuite::create();
    m_testTempDir = std::filesystem::temp_directory_path() / std::format("ezdsl_codegen_test_{}", ++s_testCounter);
    std::filesystem::create_directories(m_testTempDir);
}

// GoogleTest TearDown hook: removes the temp directory and destroys the suite.
void EzDslCodeGeneratorsTestSuiteAsGtest::TearDown()
{
    std::error_code ec;
    std::filesystem::remove_all(m_testTempDir, ec);
    EzDslCodeGeneratorsTestSuite::destroy();
    Test::TearDown();
}
