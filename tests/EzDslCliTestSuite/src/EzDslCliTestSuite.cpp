#include "EzDslCliTestSuite.h"
#include "EzTargetsX86_64Dsl.h"

#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Sema/SymbolTable.h"
#include "SourceManager/SourceManager.h"

#include <cstdio>
#include <format>
#include <fstream>
#include <sstream>

#if defined(_WIN32) || defined(_WIN64)
#define popen _popen
#define pclose _pclose
#endif

// Retrieves the active diagnostic collector.
DiagnosticCollector *EzDslCliTestSuite::getDiagCollector() { return m_diagnosticCollector; }

// Retrieves the diagnostic logger.
DiagnosticLogger *EzDslCliTestSuite::getDiagLogger() { return m_diagnosticLogger; }

/**
 * Registers an in-memory source buffer with the source manager and returns a
 * ParseContext bound to the test allocator and diagnostics. Throws if the
 * source name was already registered.
 */
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

// Retrieves the source manager.
SourceManager *EzDslCliTestSuite::getSourceManager() { return m_sourceManager; }

// Retrieves the symbol table.
SymbolTable *EzDslCliTestSuite::getSymbolTable() { return m_symbolTable; }

/**
 * Allocates the diagnostic collector, logger, source manager, and symbol table
 * from the internal PMR buffer resource and enables trace/debug diagnostics.
 */
void EzDslCliTestSuite::create()
{
    EzTargets::X86_64::registerDsl();

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
 * Destroys all allocated CLI test resources and releases the PMR buffer.
 */
void EzDslCliTestSuite::destroy()
{
    std::pmr::polymorphic_allocator<> alloc(&m_allocator);
    alloc.delete_object(m_symbolTable);
    alloc.delete_object(m_sourceManager);
    alloc.delete_object(m_diagnosticCollector);
    alloc.delete_object(m_diagnosticLogger);
    m_allocator.release();
}

// Retrieves the monotonic memory resource.
std::pmr::memory_resource *EzDslCliTestSuite::getAllocator() { return &m_allocator; }

/**
 * Reads the entire contents of a file into a string, returning an empty string
 * if the file cannot be opened.
 */
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

/**
 * Writes the given content to a file, creating parent directories as needed.
 * Returns true if the write succeeded.
 */
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

/**
 * Walks up from the current directory (up to six levels) looking for the
 * repository root, identified by the presence of EzMir/types.tyf.
 */
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

// Returns the path to the bundled EzMir type definitions file.
std::filesystem::path EzDslCliTestSuite::getEzMirTypesPath() { return findEzPackerRoot() / "EzMir" / "types.tyf"; }

// Returns the path to the bundled EzMir IR instruction definition file.
std::filesystem::path EzDslCliTestSuite::getEzMirInstructionsPath()
{
    return findEzPackerRoot() / "EzMir" / "instructions.irdf";
}

/**
 * Searches common build/output locations for the compiled EzDslCli executable,
 * returning an empty path if none is found.
 */
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

/**
 * Creates a file with the given name and content inside dir, returning its path.
 */
std::filesystem::path EzDslCliTestSuite::createTempFile(const std::filesystem::path &dir,
                                                        const std::string &fileName,
                                                        const std::string &content)
{
    auto target = dir / fileName;
    writeFileContent(target, content);
    return target;
}

/**
 * Prepends the dummy program name and parses args through the real CLI parser
 * without spawning a process, returning parsed options or nullopt on error.
 */
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

/**
 * Runs the CLI driver in-process with the given options and returns its result.
 */
Cli::DriverResult EzDslCliTestSuite::runDriver(Cli::CliOptions options)
{
    Cli::Driver driver(std::move(options));
    return driver.run();
}

/**
 * Executes the EzDslCli binary as a subprocess with the given arguments,
 * capturing combined stdout/stderr and returning the exit code
 * (-1 if the executable cannot be found or launched).
 */
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

    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
    {
        return -1;
    }

    char buffer[512];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        stdOut += buffer;
    }

    return pclose(pipe);
}

// GoogleTest SetUp hook: initializes the suite and creates an isolated temp directory.
void EzDslCliTestSuiteAsGtest::SetUp()
{
    Test::SetUp();
    EzDslCliTestSuite::create();
    m_testTempDir = std::filesystem::temp_directory_path() / std::format("ezdsl_cli_test_{}", ++s_testCounter);
    std::filesystem::create_directories(m_testTempDir);
}

// GoogleTest TearDown hook: removes the temp directory and destroys the suite.
void EzDslCliTestSuiteAsGtest::TearDown()
{
    std::error_code ec;
    std::filesystem::remove_all(m_testTempDir, ec);
    EzDslCliTestSuite::destroy();
    Test::TearDown();
}
