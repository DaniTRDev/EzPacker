#include "EzDslCliTestSuite.h"

#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Sema/SymbolTable.h"
#include "SourceManager/SourceManager.h"

#include <cstdio>
#include <format>
#include <fstream>
#include <sstream>

DiagnosticCollector *EzDslCliTestSuite::getDiagCollector()
{
    return m_diagnosticCollector;
}

DiagnosticLogger *EzDslCliTestSuite::getDiagLogger()
{
    return m_diagnosticLogger;
}

ParseContext EzDslCliTestSuite::createParseContextFromBuff(const std::string &sourceName,
                                                           const std::string &sourceContent)
{
    size_t sourceId = m_sourceManager->addSourceContent(sourceName, sourceContent);
    if (sourceId == 0)
    {
        throw std::runtime_error("Failed to create ParseContext because source was already added: " + sourceName);
    }

    return ParseContext(m_diagnosticCollector, m_sourceManager, sourceId, &m_allocator);
}

SourceManager *EzDslCliTestSuite::getSourceManager()
{
    return m_sourceManager;
}

SymbolTable *EzDslCliTestSuite::getSymbolTable()
{
    return m_symbolTable;
}

void EzDslCliTestSuite::create()
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

void EzDslCliTestSuite::destroy()
{
    std::pmr::polymorphic_allocator<> alloc(&m_allocator);
    alloc.delete_object(m_symbolTable);
    alloc.delete_object(m_sourceManager);
    alloc.delete_object(m_diagnosticCollector);
    alloc.delete_object(m_diagnosticLogger);
    m_allocator.release();
}

std::pmr::memory_resource *EzDslCliTestSuite::getAllocator()
{
    return &m_allocator;
}

std::string EzDslCliTestSuite::readFileContent(const std::filesystem::path &filePath)
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

bool EzDslCliTestSuite::writeFileContent(const std::filesystem::path &filePath, const std::string &content)
{
    if (filePath.has_parent_path())
    {
        std::filesystem::create_directories(filePath.parent_path());
    }
    std::ofstream file(filePath, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }
    file << content;
    return file.good();
}

std::filesystem::path EzDslCliTestSuite::findEzPackerRoot()
{
    std::filesystem::path cur = std::filesystem::current_path();
    for (int i = 0; i < 6; ++i)
    {
        if (std::filesystem::exists(cur / "EzMir" / "types.tyf"))
        {
            return cur;
        }
        if (cur.has_parent_path() && cur.parent_path() != cur)
        {
            cur = cur.parent_path();
        }
        else
        {
            break;
        }
    }
    return std::filesystem::current_path();
}

std::filesystem::path EzDslCliTestSuite::getEzMirTypesPath()
{
    return findEzPackerRoot() / "EzMir" / "types.tyf";
}

std::filesystem::path EzDslCliTestSuite::getEzMirInstructionsPath()
{
    return findEzPackerRoot() / "EzMir" / "instructions.irdf";
}

std::filesystem::path EzDslCliTestSuite::findEzDslCliExe()
{
    auto root = findEzPackerRoot();
    std::vector<std::filesystem::path> candidates = {
        root / "cmake-build-debug" / "bin" / "EzDslCli.exe",
        root / "cmake-build-debug" / "bin" / "EzDslCli",
        root / "bin" / "EzDslCli.exe",
        root / "bin" / "EzDslCli",
        std::filesystem::current_path() / "bin" / "EzDslCli.exe",
        std::filesystem::current_path() / "EzDslCli.exe",
    };
    for (const auto &p : candidates)
    {
        if (std::filesystem::exists(p))
        {
            return p;
        }
    }
    return {};
}

std::filesystem::path EzDslCliTestSuite::createTempFile(const std::filesystem::path &dir,
                                                        const std::string &fileName,
                                                        const std::string &content)
{
    auto target = dir / fileName;
    writeFileContent(target, content);
    return target;
}

std::optional<Cli::CliOptions> EzDslCliTestSuite::parseArgs(const std::vector<std::string> &args,
                                                            std::string &errorMessage)
{
    Cli::CommandLineParser parser;
    std::vector<std::string> fullArgs;
    fullArgs.reserve(args.size() + 1);
    fullArgs.push_back("EzDslCli");
    fullArgs.insert(fullArgs.end(), args.begin(), args.end());

    std::vector<char *> argv;
    argv.reserve(fullArgs.size());
    for (auto &arg : fullArgs)
    {
        argv.push_back(arg.data());
    }

    return parser.parse(static_cast<int>(argv.size()), argv.data(), errorMessage);
}

Cli::DriverResult EzDslCliTestSuite::runDriver(Cli::CliOptions options)
{
    Cli::Driver driver(std::move(options));
    return driver.run();
}

int EzDslCliTestSuite::runCliProcess(const std::vector<std::string> &args, std::string &stdOut)
{
    auto exePath = findEzDslCliExe();
    if (exePath.empty())
    {
        return -1;
    }

    std::string cmd = "\"" + exePath.string() + "\"";
    for (const auto &arg : args)
    {
        if (arg.find(' ') != std::string::npos)
        {
            cmd += " \"" + arg + "\"";
        }
        else
        {
            cmd += " " + arg;
        }
    }
    cmd += " 2>&1";

    FILE *pipe = _popen(cmd.c_str(), "r");
    if (!pipe)
    {
        return -1;
    }

    char buffer[512];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        stdOut += buffer;
    }

    return _pclose(pipe);
}

void EzDslCliTestSuiteAsGtest::SetUp()
{
    Test::SetUp();
    EzDslCliTestSuite::create();
    m_testTempDir = std::filesystem::temp_directory_path() / std::format("ezdsl_cli_test_{}", ++s_testCounter);
    std::filesystem::create_directories(m_testTempDir);
}

void EzDslCliTestSuiteAsGtest::TearDown()
{
    std::error_code ec;
    std::filesystem::remove_all(m_testTempDir, ec);
    EzDslCliTestSuite::destroy();
    Test::TearDown();
}
